package de.tierwohlteam.android.sniffingdetectorapp.viewmodels

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import de.tierwohlteam.android.sniffingdetectorapp.blueToothTools.SniffingDetector
import de.tierwohlteam.android.sniffingdetectorapp.blueToothTools.SniffingDetectorStatus
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.asSharedFlow
import kotlinx.coroutines.launch

class MainViewModel : ViewModel() {
    private val _sniffingDetector: MutableSharedFlow<SniffingDetectorStatus> = MutableSharedFlow(replay = 0)
    val sniffingDetector = _sniffingDetector.asSharedFlow()

    init {
        viewModelScope.launch {
            SniffingDetector.sniffingStatus.collect {
                _sniffingDetector.emit(it)
            }
        }
    }

}