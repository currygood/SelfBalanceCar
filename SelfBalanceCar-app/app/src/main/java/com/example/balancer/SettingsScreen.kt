package com.example.balancer

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Text
import androidx.compose.material3.MaterialTheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.getValue
import androidx.compose.runtime.setValue
import androidx.compose.runtime.collectAsState
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp

@Composable
fun SettingsScreen(btManager: BluetoothManager, currentLang: String?, onChangeLanguage: (String) -> Unit, onBack: () -> Unit) {
    var pidEnabled by remember { mutableStateOf(false) }
    val isConnected by btManager.isConnected.collectAsState()

    Column(modifier = Modifier.fillMaxSize().background(Color(0xFF1E1E2E)).padding(16.dp), horizontalAlignment = Alignment.Start) {
        Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween, verticalAlignment = Alignment.CenterVertically) {
            Text(stringResource(id = R.string.settings), style = MaterialTheme.typography.titleLarge.copy(fontWeight = FontWeight.Bold), color = Color.White)
            Button(onClick = { onBack() }, shape = RoundedCornerShape(8.dp)) { Text(stringResource(id = R.string.back)) }
        }

        Spacer(modifier = Modifier.height(24.dp))

        Text(stringResource(id = R.string.language), style = MaterialTheme.typography.titleMedium, color = Color.White)
        Spacer(modifier = Modifier.height(12.dp))
        Row(horizontalArrangement = Arrangement.spacedBy(12.dp)) {
            Button(
                onClick = { onChangeLanguage("en") },
                colors = ButtonDefaults.buttonColors(
                    containerColor = if (currentLang == "en") Color(0xFF2196F3) else Color(0xFF555577)
                ),
                shape = RoundedCornerShape(8.dp)
            ) {
                Text(stringResource(id = R.string.lang_en), fontWeight = if (currentLang == "en") FontWeight.Bold else FontWeight.Normal)
            }
            Button(
                onClick = { onChangeLanguage("zh") },
                colors = ButtonDefaults.buttonColors(
                    containerColor = if (currentLang == "zh") Color(0xFF2196F3) else Color(0xFF555577)
                ),
                shape = RoundedCornerShape(8.dp)
            ) {
                Text(stringResource(id = R.string.lang_zh), fontWeight = if (currentLang == "zh") FontWeight.Bold else FontWeight.Normal)
            }
        }

        Spacer(modifier = Modifier.height(32.dp))

        Text("PID", style = MaterialTheme.typography.titleMedium, color = Color.White)
        Spacer(modifier = Modifier.height(12.dp))
        Row(horizontalArrangement = Arrangement.spacedBy(12.dp)) {
            Button(
                onClick = {
                    val frame = ProtocolManager.buildFrame(0x07, 0x01, 0x00)
                    btManager.writeFrame(frame) { ok -> if (ok) pidEnabled = true }
                },
                colors = ButtonDefaults.buttonColors(
                    containerColor = if (pidEnabled) Color(0xFF4CAF50) else Color(0xFF555577)
                ),
                shape = RoundedCornerShape(8.dp),
                enabled = isConnected
            ) {
                Text(stringResource(id = R.string.pid_enable), fontWeight = if (pidEnabled) FontWeight.Bold else FontWeight.Normal)
            }
            Button(
                onClick = {
                    val frame = ProtocolManager.buildFrame(0x07, 0x00, 0x00)
                    btManager.writeFrame(frame) { ok -> if (ok) pidEnabled = false }
                },
                colors = ButtonDefaults.buttonColors(
                    containerColor = if (!pidEnabled) Color(0xFFF44336) else Color(0xFF555577)
                ),
                shape = RoundedCornerShape(8.dp),
                enabled = isConnected
            ) {
                Text(stringResource(id = R.string.pid_disable), fontWeight = if (!pidEnabled) FontWeight.Bold else FontWeight.Normal)
            }
        }
    }
}