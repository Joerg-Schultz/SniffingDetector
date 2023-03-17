package de.tierwohlteam.android.recorderapp.ui.main

import androidx.lifecycle.ViewModel
import de.tierwohlteam.android.recorderapp.models.Controller

class MainViewModel : ViewModel() {
    var controller: Controller? = null
    var isRecording = false

    fun sendRecorder() {
        if (isRecording) controller?.sendStart() else controller?.sendStop()
    }
}