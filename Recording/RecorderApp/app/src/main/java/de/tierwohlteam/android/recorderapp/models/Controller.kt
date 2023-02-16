package de.tierwohlteam.android.recorderapp.models

import android.os.Message
import android.util.Log
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.asSharedFlow

class Controller() : BTTool() {

    fun sendStart() {
        super.ConnectedThread().write("1")
    }

    fun sendStop() {
        super.ConnectedThread().write("0")
    }

    override suspend fun toolReadAction(msg: Message) {
        // Nothing to do here
    }
}