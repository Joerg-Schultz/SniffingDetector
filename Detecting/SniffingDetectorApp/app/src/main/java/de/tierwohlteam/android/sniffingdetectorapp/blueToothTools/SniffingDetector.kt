package de.tierwohlteam.android.sniffingdetectorapp.blueToothTools

import android.os.Message
import android.util.Log
import kotlinx.coroutines.flow.*

object SniffingDetector : BTTool() {

    private val _sniffingStatus = MutableSharedFlow<SniffingDetectorStatus>(replay = 1)
    val sniffingStatus = _sniffingStatus.asSharedFlow()

    override suspend fun toolReadAction(msg: Message) {
        val msgText = msg.obj.toString()
        Log.d("SniffingDetector",msgText)
        val statusText = msg.obj.toString().replace("""\W""".toRegex(), "")
        Log.d("SNIFFER", statusText)
        _sniffingStatus.emit(
            when (statusText) {
                "1" -> SniffingDetectorStatus.SNIFFING
                "0" -> SniffingDetectorStatus.NOT_SNIFFING
                else -> SniffingDetectorStatus.UNKNOWN
            }
        )
    }
}

enum class SniffingDetectorStatus {
    SNIFFING, NOT_SNIFFING, UNKNOWN
}
