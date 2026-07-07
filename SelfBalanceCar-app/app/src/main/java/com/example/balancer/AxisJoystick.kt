package com.example.balancer

import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlin.math.abs

class AxisJoystick(private val onChange: ((Float) -> Unit)? = null) {
    private val _state = MutableStateFlow(0f)
    val state: StateFlow<Float> = _state

    private val deadZone = 0.10f

    fun update(v: Float) {
        val nv = if (abs(v) <= deadZone) 0f else v.coerceIn(-1f, 1f)
        _state.value = nv
        onChange?.invoke(nv)
    }
}