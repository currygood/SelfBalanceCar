package com.example.balancer

import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothSocket
import android.content.Context
import android.content.SharedPreferences
import android.os.Build
import android.content.BroadcastReceiver
import android.content.Intent
import android.content.IntentFilter
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import java.io.IOException
import java.io.OutputStream
import java.util.UUID

class BluetoothManager(private val context: Context) {

    private val prefs: SharedPreferences = context.getSharedPreferences("sb_prefs", Context.MODE_PRIVATE)
    private val adapter: BluetoothAdapter? = (context.getSystemService(android.bluetooth.BluetoothManager::class.java))?.adapter
        ?: BluetoothAdapter.getDefaultAdapter()
    private var socket: BluetoothSocket? = null
    private var outStream: OutputStream? = null
    private var inStream: java.io.InputStream? = null
    private val scope = CoroutineScope(Dispatchers.IO)
    private val writeLock = Object()
    private var writeSerial = 0L
    private var lastWrittenSerial = 0L
    private val _discovered = MutableStateFlow<List<BluetoothDevice>>(emptyList())
    val discovered: StateFlow<List<BluetoothDevice>> = _discovered

    private val _isConnected = MutableStateFlow(false)
    val isConnected: StateFlow<Boolean> = _isConnected

    private val _receivedFrame = MutableStateFlow<ByteArray?>(null)
    val receivedFrame: StateFlow<ByteArray?> = _receivedFrame

    private var discoveryReceiver: BroadcastReceiver? = null
    private var isDiscovering = false

    companion object {
        val SPP_UUID: UUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB")
        const val PREF_LAST_MAC = "last_mac"
    }

    fun isBluetoothAvailable(): Boolean {
        return adapter != null
    }

    fun getPairedDevices(): List<BluetoothDevice> {
        return try {
            adapter?.bondedDevices?.toList() ?: emptyList()
        } catch (e: SecurityException) {
            emptyList()
        } catch (e: Exception) {
            emptyList()
        }
    }

    @SuppressLint("MissingPermission")
    fun startDiscovery() {
        if (adapter == null || isDiscovering) return
        discoveryReceiver = object : BroadcastReceiver() {
            override fun onReceive(context: Context?, intent: Intent?) {
                if (intent == null) return
                when (intent.action) {
                    BluetoothDevice.ACTION_FOUND -> {
                        val device: BluetoothDevice? = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE)
                        device?.let {
                            val list = _discovered.value.toMutableList()
                            if (list.none { d -> d.address == it.address }) {
                                list.add(it)
                                _discovered.value = list
                            }
                        }
                    }
                    BluetoothAdapter.ACTION_DISCOVERY_FINISHED -> {
                        isDiscovering = false
                    }
                }
            }
        }
        val filter = IntentFilter()
        filter.addAction(BluetoothDevice.ACTION_FOUND)
        filter.addAction(BluetoothAdapter.ACTION_DISCOVERY_FINISHED)
        context.registerReceiver(discoveryReceiver, filter)
        adapter.startDiscovery()
        isDiscovering = true
    }

    fun stopDiscovery() {
        try {
            if (isDiscovering) {
                adapter?.cancelDiscovery()
                isDiscovering = false
            }
            discoveryReceiver?.let { context.unregisterReceiver(it) }
        } catch (e: Exception) {
        }
        discoveryReceiver = null
    }

    fun saveLastMac(mac: String) {
        prefs.edit().putString(PREF_LAST_MAC, mac).apply()
    }

    fun loadLastMac(): String? = prefs.getString(PREF_LAST_MAC, null)

    @SuppressLint("MissingPermission")
    suspend fun connect(mac: String, onConnected: (() -> Unit)? = null, onDisconnected: (() -> Unit)? = null): Boolean
    {
        return withContext(Dispatchers.IO) {
            try {
                disconnect()
                val adapterLocal = adapter ?: return@withContext false
                val device = adapterLocal.getRemoteDevice(mac)
                socket = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                    device.createInsecureRfcommSocketToServiceRecord(SPP_UUID)
                } else {
                    device.createRfcommSocketToServiceRecord(SPP_UUID)
                }
                adapterLocal.cancelDiscovery()
                socket?.connect()
                outStream = socket?.outputStream
                inStream = socket?.inputStream
                saveLastMac(mac)
                _isConnected.value = true
                startReading()
                onConnected?.invoke()
                true
            } catch (e: IOException) {
                e.printStackTrace()
                disconnect()
                onDisconnected?.invoke()
                false
            }
        }
    }

    private var readJob: Job? = null

    private fun startReading() {
        stopReading()
        readJob = scope.launch {
            val input = inStream ?: return@launch
            val buffer = ByteArray(6)
            var idx = 0
            try {
                while (isActive) {
                    val b = input.read()
                    if (b < 0) break
                    val by = (b and 0xFF)
                    if (idx == 0 && by != 0xAA) {
                        continue
                    }
                    buffer[idx] = by.toByte()
                    idx++
                    if (idx == 6) {
                        var sum = 0
                        for (i in 0 until 5) sum += (buffer[i].toInt() and 0xFF)
                        val chk = (sum and 0xFF).toByte()
                        if (chk == buffer[5]) {
                            _receivedFrame.value = buffer.copyOf()
                        }
                        idx = 0
                    }
                }
            } catch (e: Exception) {
                e.printStackTrace()
                disconnect()
            }
        }
    }

    private fun stopReading() {
        readJob?.cancel()
        readJob = null
    }

    fun writeFrame(frame: ByteArray, onResult: ((Boolean) -> Unit)? = null) {
        val serial = synchronized(writeLock) {
            writeSerial++
            writeSerial
        }
        scope.launch {
            val ok = try {
                synchronized(writeLock) {
                    if (serial < lastWrittenSerial) {
                        onResult?.invoke(true)
                        return@launch
                    }
                    lastWrittenSerial = serial
                    outStream?.write(frame)
                    outStream?.flush()
                }
                true
            } catch (e: Exception) {
                e.printStackTrace()
                disconnect()
                false
            }
            onResult?.invoke(ok)
        }
    }

    fun writeFrameImmediate(frame: ByteArray) {
        synchronized(writeLock) {
            writeSerial++
            lastWrittenSerial = writeSerial
            try {
                outStream?.write(frame)
                outStream?.flush()
            } catch (e: Exception) {
                e.printStackTrace()
            }
        }
    }

    fun disconnect() {
        stopReading()
        try {
            outStream?.close()
        } catch (e: Exception) {
        }
        try {
            socket?.close()
        } catch (e: Exception) {
        }
        socket = null
        outStream = null
        inStream = null
        _isConnected.value = false
    }
}