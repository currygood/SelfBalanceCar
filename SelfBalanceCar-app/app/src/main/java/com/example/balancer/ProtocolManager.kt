package com.example.balancer

import kotlin.math.roundToInt

/**
 * ProtocolManager 负责将控制值打包成帧并计算校验和。
 * 帧结构: [0xAA, Cmd, Throttle, Steering, Reserve, Checksum]
 */
object ProtocolManager {

    private const val FRAME_HEADER: Byte = 0xAA.toByte()

    fun buildFrame(cmd: Int, throttle: Int, steering: Int, reserve: Int = 0): ByteArray
    {
        val c = (cmd and 0xFF).toByte()
        val t = throttle.coerceIn(-128, 127)
        val s = steering.coerceIn(-128, 127)
        val r = (reserve and 0xFF).toByte()
        val frame = ByteArray(6)
        frame[0] = FRAME_HEADER
        frame[1] = c
        frame[2] = t.toByte()
        frame[3] = s.toByte()
        frame[4] = r
        frame[5] = checksum(frame)
        return frame
    }

    private fun checksum(frame5: ByteArray): Byte {
        var sum = 0
        for (i in 0 until 5) {
            sum += (frame5[i].toInt() and 0xFF)
        }
        return (sum and 0xFF).toByte()
    }

    fun mapControlToByte(value: Float, gain: Float = 1f): Int {
        val scaled = value * gain
        val clamped = scaled.coerceIn(-1f, 1f)
        val mapped = (clamped * 128f).roundToInt().coerceIn(-128, 127)
        return mapped
    }

    fun pidToBytes(kp: Float, ki: Float, kd: Float): ByteArray {
        // Protocol: Kp(val*10), Ki(val*100), Kd(val*10)
        // Use nearest-integer rounding to encode values strictly per protocol
        val kpInt = (kp * 10f).roundToInt().coerceIn(0, 255)
        val kiInt = (ki * 100f).roundToInt().coerceIn(0, 255)
        val kdInt = (kd * 10f).roundToInt().coerceIn(0, 255)
        return byteArrayOf(kpInt.toByte(), kiInt.toByte(), kdInt.toByte())
    }
}