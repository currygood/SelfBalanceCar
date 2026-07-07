package com.example.balancer

import android.Manifest
import android.bluetooth.BluetoothAdapter
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.app.AlertDialog
import android.content.Intent
import android.net.Uri
import android.provider.Settings
import java.util.Locale
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.Text
import androidx.compose.material3.Surface
import androidx.compose.material3.MaterialTheme
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch
import android.util.Log
import java.util.concurrent.atomic.AtomicBoolean

class MainActivity : ComponentActivity() {

    private lateinit var btManager: BluetoothManager

    private val running = AtomicBoolean(false)
    private var sendThread: Thread? = null

    private val throttleJoystick = AxisJoystick()
    private val steeringJoystick = AxisJoystick()

    private val prefs by lazy { getSharedPreferences("settings", MODE_PRIVATE) }

    private val requiredPermissions: Array<String>
        get() {
            val perms = mutableListOf<String>()
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                perms += Manifest.permission.BLUETOOTH_SCAN
                perms += Manifest.permission.BLUETOOTH_CONNECT
            } else {
                perms += Manifest.permission.ACCESS_FINE_LOCATION
            }
            return perms.toTypedArray()
        }

    private val requestPermissionLauncher = registerForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) { _ ->
        val perms = requiredPermissions
        val allGranted = perms.all { checkSelfPermissionSafe(it) == PackageManager.PERMISSION_GRANTED }
        if (allGranted) {
            discoverAndConnect()
        } else {
            val permanentlyDenied = perms.any { perm ->
                checkSelfPermissionSafe(perm) != PackageManager.PERMISSION_GRANTED && !shouldShowRequestPermissionRationale(perm)
            }
            showPermissionDeniedDialog(permanentlyDenied)
        }
    }

    override fun onCreate(savedInstanceState: Bundle?)
    {
        super.onCreate(savedInstanceState)

        btManager = BluetoothManager(this)

        applySavedLocale()

        startSendLoop()

        setContent {
            val navController = rememberNavController()
            val permsGranted = requiredPermissions.all { perm -> checkSelfPermissionSafe(perm) == PackageManager.PERMISSION_GRANTED }
            Surface(modifier = Modifier.fillMaxSize(), color = MaterialTheme.colorScheme.background) {
                NavHost(navController = navController, startDestination = "main") {
                    composable("main") {
                        MainScreen(
                            btManager = btManager,
                            throttleJoystick = throttleJoystick,
                            steeringJoystick = steeringJoystick,
                            onOpenSettings = { navController.navigate("settings") },
                            onOpenConnect = { navController.navigate("connect") },
                            onOpenPID = { navController.navigate("pid") },
                            permissionGranted = permsGranted,
                            onRequestPermissions = { requestPermissions() }
                        )
                    }
                    composable("connect") {
                        ConnectScreen(btManager = btManager, throttleJoystick = throttleJoystick, steeringJoystick = steeringJoystick, onBack = { navController.popBackStack() })
                    }
                    composable("pid") {
                        PIDScreen(btManager = btManager, onBack = { navController.popBackStack() })
                    }
                    composable("settings") {
                        SettingsScreen(btManager = btManager, currentLang = prefs.getString("lang", null), onChangeLanguage = { lang ->
                            prefs.edit().putString("lang", lang).apply()
                            setLocale(lang)
                            recreate()
                        }, onBack = { navController.popBackStack() })
                    }
                }
            }
        }

        val permsGranted = requiredPermissions.all { perm -> checkSelfPermissionSafe(perm) == PackageManager.PERMISSION_GRANTED }
        if (permsGranted) {
            discoverAndConnect()
        } else {
            requestPermissions()
        }
    }

    private fun startSendLoop() {
        if (running.getAndSet(true)) return
        sendThread = Thread {
            var lastT = 0f
            var lastS = 0f
            var lastChangeTime = System.currentTimeMillis()
            while (running.get()) {
                try {
                    Thread.sleep(50)
                    if (!btManager.isConnected.value) continue
                    val t = throttleJoystick.state.value
                    val s = steeringJoystick.state.value
                    if (t != lastT || s != lastS) {
                        lastChangeTime = System.currentTimeMillis()
                        lastT = t
                        lastS = s
                    }
                    val idle = System.currentTimeMillis() - lastChangeTime > 200
                    val frame: ByteArray
                    if (t == 0f && s == 0f) {
                        frame = ProtocolManager.buildFrame(0x01, 0, 0)
                        btManager.writeFrameImmediate(frame)
                        Thread.sleep(10)
                        btManager.writeFrameImmediate(ProtocolManager.buildFrame(0x02, 0, 0))
                        Log.d("SBControl", "STOP: t=0 s=0")
                    } else if (idle) {
                        throttleJoystick.update(0f)
                        steeringJoystick.update(0f)
                        lastT = 0f
                        lastS = 0f
                        lastChangeTime = System.currentTimeMillis()
                        frame = ProtocolManager.buildFrame(0x01, 0, 0)
                        btManager.writeFrameImmediate(frame)
                        Thread.sleep(10)
                        btManager.writeFrameImmediate(ProtocolManager.buildFrame(0x02, 0, 0))
                        Log.d("SBControl", "STOP(auto): idle > 200ms, forced reset")
                    } else {
                        val tByte = ProtocolManager.mapControlToByte(t)
                        val sByte = ProtocolManager.mapControlToByte(s, gain = 10f)
                        frame = ProtocolManager.buildFrame(0x01, tByte, sByte)
                        btManager.writeFrameImmediate(frame)
                        Log.d("SBControl", "MOVE: tByte=$tByte sByte=$sByte")
                    }
                } catch (e: InterruptedException) {
                    break
                } catch (e: Exception) {
                    Log.e("SBControl", "send error", e)
                }
            }
        }.apply {
            isDaemon = true
            start()
        }
    }

    private fun bytesToHex(bytes: ByteArray): String {
        return bytes.joinToString(" ") { String.format("%02X", it) }
    }

    private fun requestPermissions() {
        val missing = requiredPermissions.filter { perm -> checkSelfPermissionSafe(perm) != PackageManager.PERMISSION_GRANTED }
        if (missing.isNotEmpty()) {
            requestPermissionLauncher.launch(missing.toTypedArray())
        }
    }

    private fun showPermissionDeniedDialog(permanentlyDenied: Boolean) {
        val builder = AlertDialog.Builder(this)
        builder.setTitle(getString(R.string.refresh_failed_title))
        builder.setMessage(getString(R.string.refresh_failed_message))
        if (permanentlyDenied) {
            builder.setPositiveButton(getString(R.string.open_settings)) { _, _ ->
                val intent = Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS)
                intent.data = Uri.fromParts("package", packageName, null)
                startActivity(intent)
            }
            builder.setNegativeButton(getString(R.string.retry)) { _, _ -> requestPermissions() }
        } else {
            builder.setPositiveButton(getString(R.string.retry)) { _, _ -> requestPermissions() }
            builder.setNegativeButton(getString(R.string.open_settings)) { _, _ ->
                val intent = Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS)
                intent.data = Uri.fromParts("package", packageName, null)
                startActivity(intent)
            }
        }
        builder.setCancelable(true)
        builder.show()
    }

    private fun applySavedLocale() {
        val lang = prefs.getString("lang", null)
        if (!lang.isNullOrEmpty()) {
            setLocale(lang)
        }
    }

    private fun setLocale(lang: String) {
        try {
            val locale = Locale(lang)
            Locale.setDefault(locale)
            val res = resources
            val config = res.configuration
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
                config.setLocale(locale)
            } else {
                @Suppress("DEPRECATION")
                config.locale = locale
            }
            res.updateConfiguration(config, res.displayMetrics)
        } catch (e: Exception) {
        }
    }

    private fun checkSelfPermissionSafe(permission: String): Int {
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) checkSelfPermission(permission) else PackageManager.PERMISSION_GRANTED
    }

    private fun discoverAndConnect() {
        val last = btManager.loadLastMac()
        val toConnect = last ?: btManager.getPairedDevices().firstOrNull()?.address
        if (toConnect != null) {
            CoroutineScope(Dispatchers.Main).launch {
                btManager.connect(toConnect)
            }
        }
    }

    override fun onPause() {
        super.onPause()
        try {
            btManager.writeFrameImmediate(ProtocolManager.buildFrame(0x02, 0, 0))
        } catch (e: Exception) {
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        running.set(false)
        sendThread?.interrupt()
    }
}