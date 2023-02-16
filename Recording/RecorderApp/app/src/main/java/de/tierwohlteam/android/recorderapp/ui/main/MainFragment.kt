package de.tierwohlteam.android.recorderapp.ui.main

import android.bluetooth.BluetoothDevice
import android.content.Context
import androidx.lifecycle.lifecycleScope
import android.os.Bundle
import androidx.fragment.app.Fragment
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.lifecycle.ViewModelProvider
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import com.google.android.material.snackbar.Snackbar
import de.tierwohlteam.android.recorderapp.R
import de.tierwohlteam.android.recorderapp.databinding.FragmentMainBinding
import de.tierwohlteam.android.recorderapp.models.Controller
import kotlinx.coroutines.launch

class MainFragment : Fragment() {

    private var _binding: FragmentMainBinding? = null
    // This property is only valid between onCreateView and
// onDestroyView.
    private val binding get() = _binding!!
    companion object {
        fun newInstance() = MainFragment()
    }

    private lateinit var viewModel: MainViewModel
    private var isRecording = false

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        viewModel = ViewModelProvider(this).get(MainViewModel::class.java)
        _binding = FragmentMainBinding.inflate(inflater, container, false)
        val view = binding.root
        return view
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        binding.btnConnectBT.setOnClickListener {
            context?.let {
                    it1 -> connectDialog(it1) }
        }

        binding.btnStartstop.setOnClickListener {
            isRecording = !isRecording
            viewModel.sendRecorder(isRecording)
            binding.btnStartstop.text = if (isRecording) "Stop Recording" else "Start Recording"
        }
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }

    private fun connectDialog(context: Context) {
        val controller = Controller()
        val pairedDevices = controller.getPairedDevices()
        var selectedDevice: BluetoothDevice? = pairedDevices.firstOrNull()
        viewLifecycleOwner.lifecycleScope.launch {
            controller.connectionMessage.collect {
                Snackbar.make(requireView(), it, Snackbar.LENGTH_LONG).show()
                /*
                if (it.contains("failed")) {
                    with(sharedPrefs.edit()) {
                        putBoolean(key, false)
                        apply()
                    }
                }
                */
            }
        }
        MaterialAlertDialogBuilder(context)
            .setTitle(resources.getString(R.string.paired))
            .setNeutralButton(resources.getString(R.string.cancel)) { dialog, which ->
            }
            .setPositiveButton(resources.getString(R.string.ok)) { dialog, which ->
                selectedDevice?.let {
                    viewModel.controller = controller
                    lifecycleScope.launch {
                        viewModel.controller!!.startCommunication(it)
                    }
                }
            }
            .setSingleChoiceItems(pairedDevices.map { it.name } .toTypedArray(), 0) { dialog, which ->
                selectedDevice = pairedDevices[which]
            }
            .show()
    }
}