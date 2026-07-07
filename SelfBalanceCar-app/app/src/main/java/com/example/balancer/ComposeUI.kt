package com.example.balancer

import android.bluetooth.BluetoothDevice
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.gestures.detectDragGestures
import androidx.compose.ui.input.pointer.PointerEventPass
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.MaterialTheme
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.IntSize
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.layout.onSizeChanged
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.TextFieldValue
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch

const val RING_ANGLE = 0
const val RING_SPEED = 1
const val RING_STEER = 2

private val AccentBlue = Color(0xFF2196F3)
private val AccentGreen = Color(0xFF4CAF50)
private val AccentRed = Color(0xFFF44336)
private val AccentOrange = Color(0xFFFF9800)
private val DarkSurface = Color(0xFF1E1E2E)
private val CardBg = Color(0xFF2A2A3C)
private val DimText = Color(0xFF8888AA)

@Composable
fun MainScreen(
    btManager: BluetoothManager,
    throttleJoystick: AxisJoystick,
    steeringJoystick: AxisJoystick,
    onOpenSettings: () -> Unit,
    onOpenConnect: () -> Unit,
    onOpenPID: () -> Unit,
    permissionGranted: Boolean,
    onRequestPermissions: () -> Unit
) {
    val isConnected by btManager.isConnected.collectAsState()

    Column(
        modifier = Modifier.fillMaxSize().background(DarkSurface).padding(12.dp),
        verticalArrangement = Arrangement.SpaceBetween
    ) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text(
                stringResource(id = R.string.app_name),
                style = MaterialTheme.typography.titleMedium.copy(fontWeight = FontWeight.Bold),
                color = Color.White
            )
            Row(verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                Canvas(modifier = Modifier.size(10.dp)) {
                    drawCircle(color = if (isConnected) AccentGreen else AccentRed)
                }
                Text(
                    if (isConnected) stringResource(id = R.string.connected) else stringResource(id = R.string.disconnected),
                    style = MaterialTheme.typography.bodySmall,
                    color = if (isConnected) AccentGreen else AccentRed
                )
            }
        }

        Row(
            modifier = Modifier.weight(1f).fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceEvenly,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Column(horizontalAlignment = Alignment.CenterHorizontally, modifier = Modifier.weight(1f)) {
                Text(
                    stringResource(id = R.string.throttle).removeSuffix(":"),
                    style = MaterialTheme.typography.labelLarge,
                    color = AccentBlue
                )
                Spacer(Modifier.height(4.dp))
                AxisJoystickControl(axisJoystick = throttleJoystick, orientation = "vertical", accentColor = AccentBlue)
            }
            Column(horizontalAlignment = Alignment.CenterHorizontally, modifier = Modifier.weight(1f)) {
                Text(
                    stringResource(id = R.string.steering).removeSuffix(":"),
                    style = MaterialTheme.typography.labelLarge,
                    color = AccentOrange
                )
                Spacer(Modifier.height(4.dp))
                AxisJoystickControl(axisJoystick = steeringJoystick, orientation = "horizontal", accentColor = AccentOrange)
            }
        }

        Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            val btnMod = Modifier.weight(1f)
            if (isConnected) {
                Button(
                    onClick = { btManager.disconnect() },
                    modifier = btnMod,
                    colors = ButtonDefaults.buttonColors(containerColor = AccentRed),
                    shape = RoundedCornerShape(8.dp)
                ) { Text(stringResource(id = R.string.disconnect), fontWeight = FontWeight.Bold) }
            } else {
                Button(
                    onClick = { onOpenConnect() },
                    modifier = btnMod,
                    colors = ButtonDefaults.buttonColors(containerColor = AccentGreen),
                    shape = RoundedCornerShape(8.dp)
                ) { Text(stringResource(id = R.string.connect), fontWeight = FontWeight.Bold) }
            }
            Button(
                onClick = { onOpenPID() },
                modifier = btnMod,
                colors = ButtonDefaults.buttonColors(containerColor = AccentBlue),
                shape = RoundedCornerShape(8.dp)
            ) { Text(stringResource(id = R.string.pid_panel), fontWeight = FontWeight.Bold) }
            Button(
                onClick = { onOpenSettings() },
                modifier = btnMod,
                colors = ButtonDefaults.buttonColors(containerColor = Color(0xFF555577)),
                shape = RoundedCornerShape(8.dp)
            ) { Text(stringResource(id = R.string.settings)) }
        }
    }
}

@Composable
fun ConnectScreen(btManager: BluetoothManager, throttleJoystick: AxisJoystick, steeringJoystick: AxisJoystick, onBack: () -> Unit) {
    val scope = rememberCoroutineScope()
    var pairedDevices by remember { mutableStateOf(btManager.getPairedDevices()) }
    val discovered by btManager.discovered.collectAsState()
    var scanning by remember { mutableStateOf(false) }

    Column(modifier = Modifier.fillMaxSize().background(DarkSurface).padding(12.dp)) {
        Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
            Button(onClick = { onBack() }, shape = RoundedCornerShape(8.dp)) { Text(stringResource(id = R.string.back)) }
            Button(onClick = { pairedDevices = btManager.getPairedDevices() }, shape = RoundedCornerShape(8.dp)) { Text(stringResource(id = R.string.refresh_paired)) }
        }
        Spacer(Modifier.height(8.dp))
        Button(
            onClick = {
                if (!scanning) btManager.startDiscovery() else btManager.stopDiscovery()
                scanning = !scanning
            },
            colors = ButtonDefaults.buttonColors(containerColor = if (!scanning) AccentBlue else AccentOrange),
            shape = RoundedCornerShape(8.dp)
        ) { Text(if (!scanning) stringResource(id = R.string.start_scan) else stringResource(id = R.string.stop_scan)) }
        Spacer(Modifier.height(12.dp))
        Text(stringResource(id = R.string.paired_devices), style = MaterialTheme.typography.titleSmall, color = AccentBlue)
        Spacer(Modifier.height(4.dp))
        LazyColumn(modifier = Modifier.weight(1f)) { items(pairedDevices) { device -> DeviceRow(device, btManager, throttleJoystick, steeringJoystick) } }
        Spacer(Modifier.height(12.dp))
        Text(stringResource(id = R.string.discovered_devices), style = MaterialTheme.typography.titleSmall, color = AccentOrange)
        Spacer(Modifier.height(4.dp))
        LazyColumn(modifier = Modifier.weight(1f)) { items(discovered) { device -> DeviceRow(device, btManager, throttleJoystick, steeringJoystick) } }
    }
}

@Composable
fun PIDScreen(btManager: BluetoothManager, onBack: () -> Unit) {
    var currentRing by remember { mutableStateOf(RING_ANGLE) }
    Column(modifier = Modifier.fillMaxSize().background(DarkSurface).padding(12.dp).verticalScroll(rememberScrollState())) {
        Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween, verticalAlignment = Alignment.CenterVertically) {
            Button(onClick = { onBack() }, shape = RoundedCornerShape(8.dp)) { Text(stringResource(id = R.string.back)) }
            Text(stringResource(id = R.string.pid_panel), style = MaterialTheme.typography.titleLarge.copy(fontWeight = FontWeight.Bold), color = Color.White)
        }
        Spacer(Modifier.height(12.dp))
        Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            val ringBtnMod = Modifier.weight(1f)

            if (currentRing == RING_ANGLE) {
                Button(onClick = { currentRing = RING_ANGLE }, modifier = ringBtnMod, colors = ButtonDefaults.buttonColors(containerColor = AccentBlue), shape = RoundedCornerShape(8.dp)) { Text(stringResource(id = R.string.ring_angle), fontWeight = FontWeight.Bold) }
            } else {
                OutlinedButton(onClick = { currentRing = RING_ANGLE }, modifier = ringBtnMod, shape = RoundedCornerShape(8.dp)) { Text(stringResource(id = R.string.ring_angle), color = DimText) }
            }
            if (currentRing == RING_SPEED) {
                Button(onClick = { currentRing = RING_SPEED }, modifier = ringBtnMod, colors = ButtonDefaults.buttonColors(containerColor = AccentGreen), shape = RoundedCornerShape(8.dp)) { Text(stringResource(id = R.string.ring_speed), fontWeight = FontWeight.Bold) }
            } else {
                OutlinedButton(onClick = { currentRing = RING_SPEED }, modifier = ringBtnMod, shape = RoundedCornerShape(8.dp)) { Text(stringResource(id = R.string.ring_speed), color = DimText) }
            }
            if (currentRing == RING_STEER) {
                Button(onClick = { currentRing = RING_STEER }, modifier = ringBtnMod, colors = ButtonDefaults.buttonColors(containerColor = AccentOrange), shape = RoundedCornerShape(8.dp)) { Text(stringResource(id = R.string.ring_steer), fontWeight = FontWeight.Bold) }
            } else {
                OutlinedButton(onClick = { currentRing = RING_STEER }, modifier = ringBtnMod, shape = RoundedCornerShape(8.dp)) { Text(stringResource(id = R.string.ring_steer), color = DimText) }
            }
        }
        Spacer(Modifier.height(12.dp))
        PIDPanel(btManager = btManager, currentRing = currentRing)
    }
}

@Composable
fun DeviceRow(device: BluetoothDevice, btManager: BluetoothManager, throttleJoystick: AxisJoystick, steeringJoystick: AxisJoystick) {
    val scope = rememberCoroutineScope()
    val isConnected by btManager.isConnected.collectAsState()
    val name = device.name ?: "Unknown"
    val lastMac = btManager.loadLastMac()
    val isThisDevice = isConnected && lastMac == device.address
    var connecting by remember { mutableStateOf(false) }
    Card(
        modifier = Modifier.fillMaxWidth().padding(vertical = 3.dp),
        colors = CardDefaults.cardColors(containerColor = CardBg),
        shape = RoundedCornerShape(8.dp)
    ) {
        Row(modifier = Modifier.fillMaxWidth().padding(horizontal = 12.dp, vertical = 8.dp), verticalAlignment = Alignment.CenterVertically) {
            Column(modifier = Modifier.weight(1f)) {
                Text(name, color = Color.White, fontWeight = FontWeight.Medium)
                Text(device.address, style = MaterialTheme.typography.bodySmall, color = DimText)
            }
            if (isThisDevice) {
                Text(stringResource(id = R.string.connected), color = AccentGreen, fontWeight = FontWeight.Bold, fontSize = 12.sp)
            } else {
                Button(
                    onClick = {
                        if (connecting) return@Button
                        connecting = true
                        scope.launch(Dispatchers.IO) {
                            btManager.connect(device.address,
                                onConnected = {
                                    connecting = false
                                },
                                onDisconnected = {
                                    connecting = false
                                })
                        }
                    },
                    colors = ButtonDefaults.buttonColors(containerColor = if (connecting) Color(0xFF888888) else AccentGreen),
                    shape = RoundedCornerShape(6.dp),
                    enabled = !connecting
                ) { Text(if (connecting) "..." else stringResource(id = R.string.connect), fontSize = 12.sp) }
            }
            Spacer(Modifier.width(6.dp))
            Button(
                onClick = { btManager.disconnect() },
                colors = ButtonDefaults.buttonColors(containerColor = AccentRed),
                shape = RoundedCornerShape(6.dp)
            ) { Text(stringResource(id = R.string.disconnect), fontSize = 12.sp) }
        }
    }
}

@Composable
fun AxisJoystickControl(axisJoystick: AxisJoystick, orientation: String, accentColor: Color = AccentBlue) {
    val sizeDp = 130.dp
    var canvasSize by remember { mutableStateOf(IntSize.Zero) }
    var center by remember { mutableStateOf(Offset.Zero) }
    var knob by remember { mutableStateOf(Offset.Zero) }
    Box(
        modifier = Modifier
            .size(sizeDp)
            .border(2.dp, accentColor.copy(alpha = 0.5f), RoundedCornerShape(16.dp))
            .background(CardBg, RoundedCornerShape(16.dp))
            .onSizeChanged { size ->
                canvasSize = size
                if (center == Offset.Zero) {
                    center = Offset(size.width / 2f, size.height / 2f)
                    knob = center
                }
            },
        contentAlignment = Alignment.Center
    ) {
        Canvas(modifier = Modifier
            .fillMaxSize()
            .pointerInput(Unit) {
                detectDragGestures(
                    onDragStart = { _ ->
                        if (center == Offset.Zero && canvasSize != IntSize.Zero) {
                            center = Offset(canvasSize.width / 2f, canvasSize.height / 2f)
                            knob = center
                        }
                    },
                    onDrag = { _, dragAmount ->
                        if (center == Offset.Zero && canvasSize != IntSize.Zero) {
                            center = Offset(canvasSize.width / 2f, canvasSize.height / 2f)
                            knob = center
                        }
                        val new = knob + dragAmount
                        val max = minOf(center.x, center.y)
                        val v = new - center
                        val dist = kotlin.math.sqrt(v.x * v.x + v.y * v.y)
                        val clamped = if (dist > max) center + v * (max / dist) else new
                        val newKnob = if (orientation == "vertical") Offset(center.x, clamped.y) else Offset(clamped.x, center.y)
                        knob = newKnob
                        val nx = ((knob.x - center.x) / max).coerceIn(-1f, 1f)
                        val ny = ((center.y - knob.y) / max).coerceIn(-1f, 1f)
                        val value = if (orientation == "vertical") ny else nx
                        axisJoystick.update(value)
                    },
                    onDragEnd = {
                        knob = center
                        axisJoystick.update(0f)
                    },
                    onDragCancel = {
                        knob = center
                        axisJoystick.update(0f)
                    }
                )
            }
            .pointerInput(Unit) {
                awaitPointerEventScope {
                    while (true) {
                        val event = awaitPointerEvent(PointerEventPass.Final)
                        if (event.changes.all { !it.pressed }) {
                            knob = center
                            axisJoystick.update(0f)
                        }
                    }
                }
            }) {
            val radius = size.minDimension / 2f
            drawCircle(color = Color(0xFF333348), radius = radius)
            drawCircle(color = accentColor.copy(alpha = 0.15f), radius = radius * 0.7f)
            drawCircle(color = accentColor.copy(alpha = 0.3f), style = Stroke(width = 2f), radius = radius * 0.5f)
            if (orientation == "vertical") {
                drawLine(color = accentColor.copy(alpha = 0.2f), start = Offset(center.x, 0f), end = Offset(center.x, size.height.toFloat()), strokeWidth = 1f)
            } else {
                drawLine(color = accentColor.copy(alpha = 0.2f), start = Offset(0f, center.y), end = Offset(size.width.toFloat(), center.y), strokeWidth = 1f)
            }
            drawCircle(color = accentColor.copy(alpha = 0.3f), radius = 22f, center = knob)
            drawCircle(color = accentColor, radius = 14f, center = knob)
        }
    }
    Spacer(Modifier.height(4.dp))
    val state by axisJoystick.state.collectAsState()
    Text(
        "${"%.2f".format(state)}",
        color = accentColor,
        style = MaterialTheme.typography.labelMedium.copy(fontWeight = FontWeight.Bold)
    )
}

@Composable
fun PIDPanel(btManager: BluetoothManager, currentRing: Int) {
    val scope = rememberCoroutineScope()
    var kp by remember { mutableStateOf(TextFieldValue("0.0")) }
    var ki by remember { mutableStateOf(TextFieldValue("0.0")) }
    var kd by remember { mutableStateOf(TextFieldValue("0.0")) }
    var lastReceivedRing by remember { mutableStateOf<Int?>(null) }

    fun ringToSetCmd(ring: Int): Int = when (ring) {
        RING_ANGLE -> 0x04
        RING_SPEED -> 0x08
        RING_STEER -> 0x09
        else -> 0x04
    }

    fun ringToGetCmd(ring: Int): Int = when (ring) {
        RING_ANGLE -> 0x05
        RING_SPEED -> 0x10
        RING_STEER -> 0x11
        else -> 0x05
    }

    Card(
        modifier = Modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(containerColor = CardBg),
        shape = RoundedCornerShape(12.dp)
    ) {
        Column(modifier = Modifier.padding(12.dp)) {
            Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                OutlinedTextField(
                    value = kp, onValueChange = { kp = it },
                    label = { Text(stringResource(id = R.string.kp), color = AccentBlue) },
                    modifier = Modifier.weight(1f),
                    shape = RoundedCornerShape(8.dp),
                    singleLine = true,
                    textStyle = MaterialTheme.typography.bodyLarge.copy(color = Color.White, fontWeight = FontWeight.Bold),
                    colors = androidx.compose.material3.OutlinedTextFieldDefaults.colors(
                        focusedBorderColor = AccentBlue,
                        unfocusedBorderColor = Color(0xFF666688),
                        focusedLabelColor = AccentBlue,
                        cursorColor = AccentBlue
                    )
                )
                OutlinedTextField(
                    value = ki, onValueChange = { ki = it },
                    label = { Text(stringResource(id = R.string.ki), color = AccentBlue) },
                    modifier = Modifier.weight(1f),
                    shape = RoundedCornerShape(8.dp),
                    singleLine = true,
                    textStyle = MaterialTheme.typography.bodyLarge.copy(color = Color.White, fontWeight = FontWeight.Bold),
                    colors = androidx.compose.material3.OutlinedTextFieldDefaults.colors(
                        focusedBorderColor = AccentBlue,
                        unfocusedBorderColor = Color(0xFF666688),
                        focusedLabelColor = AccentBlue,
                        cursorColor = AccentBlue
                    )
                )
                OutlinedTextField(
                    value = kd, onValueChange = { kd = it },
                    label = { Text(stringResource(id = R.string.kd), color = AccentBlue) },
                    modifier = Modifier.weight(1f),
                    shape = RoundedCornerShape(8.dp),
                    singleLine = true,
                    textStyle = MaterialTheme.typography.bodyLarge.copy(color = Color.White, fontWeight = FontWeight.Bold),
                    colors = androidx.compose.material3.OutlinedTextFieldDefaults.colors(
                        focusedBorderColor = AccentBlue,
                        unfocusedBorderColor = Color(0xFF666688),
                        focusedLabelColor = AccentBlue,
                        cursorColor = AccentBlue
                    )
                )
            }

            Spacer(Modifier.height(12.dp))

            Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                val actionBtn = Modifier.weight(1f)
                Button(
                    onClick = {
                        val kpF = kp.text.toFloatOrNull() ?: 0f
                        val kiF = ki.text.toFloatOrNull() ?: 0f
                        val kdF = kd.text.toFloatOrNull() ?: 0f
                        val bytes = ProtocolManager.pidToBytes(kpF, kiF, kdF)
                        val cmd = ringToSetCmd(currentRing)
                        val frame = ProtocolManager.buildFrame(cmd, bytes[0].toInt() and 0xFF, bytes[1].toInt() and 0xFF, bytes[2].toInt() and 0xFF)
                        scope.launch(Dispatchers.IO) { btManager.writeFrame(frame) }
                    },
                    modifier = actionBtn,
                    colors = ButtonDefaults.buttonColors(containerColor = AccentBlue),
                    shape = RoundedCornerShape(8.dp)
                ) {
                    Text(stringResource(id = R.string.send_pid), fontWeight = FontWeight.Bold)
                }

                Button(
                    onClick = {
                        val frame = ProtocolManager.buildFrame(ringToGetCmd(currentRing), 0, 0)
                        scope.launch(Dispatchers.IO) { btManager.writeFrame(frame) }
                    },
                    modifier = actionBtn,
                    colors = ButtonDefaults.buttonColors(containerColor = AccentOrange),
                    shape = RoundedCornerShape(8.dp)
                ) {
                    Text(stringResource(id = R.string.get_pid), fontWeight = FontWeight.Bold)
                }
            }

            Spacer(Modifier.height(8.dp))

            Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                Button(
                    onClick = {
                        val frame = ProtocolManager.buildFrame(0x07, 0x01, 0)
                        scope.launch(Dispatchers.IO) { btManager.writeFrame(frame) }
                    },
                    modifier = Modifier.weight(1f),
                    colors = ButtonDefaults.buttonColors(containerColor = AccentGreen),
                    shape = RoundedCornerShape(8.dp)
                ) { Text(stringResource(id = R.string.pid_enable), fontWeight = FontWeight.Bold) }

                Button(
                    onClick = {
                        val frame = ProtocolManager.buildFrame(0x07, 0x00, 0)
                        scope.launch(Dispatchers.IO) { btManager.writeFrame(frame) }
                    },
                    modifier = Modifier.weight(1f),
                    colors = ButtonDefaults.buttonColors(containerColor = AccentRed),
                    shape = RoundedCornerShape(8.dp)
                ) { Text(stringResource(id = R.string.pid_disable), fontWeight = FontWeight.Bold) }
            }
        }
    }

    Spacer(Modifier.height(8.dp))
    val received by btManager.receivedFrame.collectAsState()
    LaunchedEffect(received) {
        received?.let { frame ->
            if (frame.size >= 6 && (frame[0].toInt() and 0xFF) == 0xAA) {
                val cmd = frame[1].toInt() and 0xFF
                val ring = when (cmd) {
                    0x05 -> RING_ANGLE
                    0x10 -> RING_SPEED
                    0x11 -> RING_STEER
                    else -> null
                }
                ring?.let {
                    lastReceivedRing = it
                    val kpEnc = frame[2].toInt() and 0xFF
                    val kiEnc = frame[3].toInt() and 0xFF
                    val kdEnc = frame[4].toInt() and 0xFF
                    val kpF = kpEnc / 10.0f
                    val kiF = kiEnc / 100.0f
                    val kdF = kdEnc / 10.0f
                    kp = TextFieldValue("${"%.2f".format(kpF)}")
                    ki = TextFieldValue("${"%.2f".format(kiF)}")
                    kd = TextFieldValue("${"%.2f".format(kdF)}")
                }
            }
        }
    }

    lastReceivedRing?.let { ring ->
        val name = when (ring) { RING_ANGLE -> stringResource(id = R.string.ring_angle); RING_SPEED -> stringResource(id = R.string.ring_speed); RING_STEER -> stringResource(id = R.string.ring_steer); else -> "" }
        Spacer(Modifier.height(4.dp))
        Text("${stringResource(id = R.string.get_pid)}: $name", color = AccentGreen, style = MaterialTheme.typography.bodySmall)
    }
}