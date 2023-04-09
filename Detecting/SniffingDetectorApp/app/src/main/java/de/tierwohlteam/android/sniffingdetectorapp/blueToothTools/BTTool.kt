package de.tierwohlteam.android.sniffingdetectorapp.blueToothTools

import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothSocket
import android.os.Handler
import android.os.Looper
import android.os.Message
import android.util.Log
import kotlinx.coroutines.*
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import java.io.IOException
import java.io.InputStream
import java.io.OutputStream

/**
 * Base class to handle BlueTooth connection
 * derived classes implement
 *  fun handleMessage(msg:Message)
 * to define the tool specific functions
 */
abstract class BTTool {
    abstract suspend fun toolReadAction(msg:Message)

    protected val _connectionMessage: MutableStateFlow<String> = MutableStateFlow(value = "")
    val connectionMessage: StateFlow<String> = _connectionMessage

    private val TAG = "BLUETOOTH_CONNECTION"
    private lateinit var createConnectThread: CreateConnectThread
    private lateinit var mmSocket: BluetoothSocket
    private lateinit var connectedThread: ConnectedThread

    // The following variables used in bluetooth handler to identify message status
    protected val CONNECTION_STATUS = 1
    protected val MESSAGE_READ = 2

    private val handler = object : Handler(Looper.getMainLooper()) {
        override fun handleMessage(msg: Message) {
            when (msg.what) {
                // If the updates come from the Thread to Create Connection
                CONNECTION_STATUS -> {
                    _connectionMessage.value = if (msg.arg1 == 1) "Bluetooth Connected" else "Connection Failed"
                }
                // If the updates come from the Thread for Data Exchange
                MESSAGE_READ -> {

                    //RunBlocking makes sense here, as these messages should be analysed
                    // immediately. You don't want to delay a click because others stuff is running
                    runBlocking(Dispatchers.Default) {
                        toolReadAction(msg)
                    }
                }
            }
        }
    }

    fun getPairedDevices(): List<BluetoothDevice> {
        // getDefaultAdapter is deprecated, but Documentation still uses it:
        // https://developer.android.com/guide/topics/connectivity/bluetooth/setup
        val bluetoothAdapter = BluetoothAdapter.getDefaultAdapter()
        if (bluetoothAdapter == null) {
            // Device doesn't support Bluetooth
        }
        return bluetoothAdapter?.bondedDevices?.toList() ?: emptyList()
    }

    suspend fun startCommunication(device: BluetoothDevice) {
        val bluetoothAdapter = BluetoothAdapter.getDefaultAdapter()
        _connectionMessage.emit("Connecting...")
        createConnectThread = CreateConnectThread(bluetoothAdapter, device.address.toString())
        createConnectThread.start()
    }
    suspend fun cancelConnection() {
        try {
            mmSocket.close()
        } catch (e: Exception) {
        }
    }
    /* ============================ Thread to Create Connection ================================= */
    inner class CreateConnectThread(bluetoothAdapter: BluetoothAdapter, address: String) : Thread() {

        init {
            // Opening connection socket with the Arduino board
            val bluetoothDevice = bluetoothAdapter.getRemoteDevice(address)
            val uuid = bluetoothDevice.uuids[0].uuid
            try {
                // Get a BluetoothSocket to connect with the given BluetoothDevice.
                // MY_UUID is the app's UUID string, also used in the server code.
                mmSocket = bluetoothDevice.createInsecureRfcommSocketToServiceRecord(uuid)
            } catch (e: IOException) {
                Log.e(TAG, "Socket's create() method failed", e)
            }
        }

        override fun run() {
            // Cancel discovery because it otherwise slows down the connection.
            val bluetoothAdapter = BluetoothAdapter.getDefaultAdapter()
            bluetoothAdapter.cancelDiscovery()
            try {
                // Connect to the Arduino board through the socket. This call blocks
                // until it succeeds or throws an exception.
                mmSocket.connect()
                handler.obtainMessage(CONNECTION_STATUS, 1, -1).sendToTarget()
            } catch (connectException: IOException) {
                // Unable to connect; close the socket and return.
                try {
                    mmSocket.close()
                    handler.obtainMessage(CONNECTION_STATUS, -1, -1).sendToTarget()
                } catch (closeException: IOException) {
                }
                return
            }

            // The connection attempt succeeded. Perform work associated with
            // the connection in a separate thread.
            // Calling for the Thread for Data Exchange (see below)
            //connectedThread = ConnectedThread(mmSocket)
            connectedThread = ConnectedThread()
            connectedThread.run()
        }

/*        // Closes the client socket and causes the thread to finish.
        // Disconnect from Arduino board
        fun cancel() {
            try {
                mmSocket.close()
            } catch (e: IOException) {
            }
        } */
    }

    /* =============================== Thread for Data Exchange ================================= */
    inner class ConnectedThread// Getting Input and Output Stream when connected to Arduino Board
    //(private val mmSocket: BluetoothSocket)
        : Thread() {
        private lateinit var mmInStream: InputStream
        private lateinit var mmOutStream: OutputStream

        init {
            try {
                mmInStream = mmSocket.inputStream
                mmOutStream = mmSocket.outputStream
            } catch (e: IOException) {
            }
        }

        // Read message from Arduino device and send it to handler in the Main Thread
        override fun run() {
            val buffer = ByteArray(1024)  // buffer store for the stream
            var bytes = 0 // bytes returned from read()
            // Keep listening to the InputStream until an exception occurs
            while (true) {
                try {
                    // Read from the InputStream
                    buffer[bytes] = mmInStream.read().toByte()

                    // Parsing the incoming data stream
                    if (buffer[bytes] == '\n'.toByte()) {
                        val arduinoMsg = String(buffer, 0, bytes)
                        handler.obtainMessage(MESSAGE_READ, arduinoMsg).sendToTarget()
                        bytes = 0
                    } else {
                        bytes++
                    }
                } catch (e: IOException) {
                    e.printStackTrace()
                    break
                }

            }
        }

        // Send command to Arduino Board
        // This method must be called from Main Thread
        fun write(input: String) {
            val bytes = input.toByteArray() //converts entered String into bytes
            try {
                mmOutStream.write(bytes)
            } catch (e: IOException) {
            }

        }
    }
}