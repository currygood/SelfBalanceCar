package com.example.balancer

import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlin.math.abs

/**
 * 简单摇杆逻辑，state 为 Pair(throttle, steering) 范围 [-1, 1]
 * 包含 10% 死区逻辑。
 */
class Joystick
{
    private val _state = MutableStateFlow(Pair(0f, 0f))
    val state: StateFlow<Pair<Float, Float>> = _state

    private val deadZone = 0.10f

    fun update(throttle: Float, steering: Float)
    {
        val t = applyDeadZone(throttle)
        val s = applyDeadZone(steering)
        _state.value = Pair(t, s)
    }

    private fun applyDeadZone(v: Float): Float {
        return if (abs(v) <= deadZone) 0f else v
    }
}
