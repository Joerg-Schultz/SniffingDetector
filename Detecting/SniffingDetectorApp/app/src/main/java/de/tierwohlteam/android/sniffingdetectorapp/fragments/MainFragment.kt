package de.tierwohlteam.android.sniffingdetectorapp.fragments

import android.Manifest
import android.bluetooth.BluetoothDevice
import android.content.Context
import android.content.pm.PackageManager
import android.graphics.Color
import android.os.Bundle
import androidx.fragment.app.Fragment
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.core.app.ActivityCompat
import androidx.fragment.app.activityViewModels
import androidx.lifecycle.lifecycleScope
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import com.google.android.material.snackbar.Snackbar
import de.tierwohlteam.android.sniffingdetectorapp.R
import de.tierwohlteam.android.sniffingdetectorapp.blueToothTools.BTTool
import de.tierwohlteam.android.sniffingdetectorapp.blueToothTools.SniffingDetector
import de.tierwohlteam.android.sniffingdetectorapp.blueToothTools.SniffingDetectorStatus
import de.tierwohlteam.android.sniffingdetectorapp.databinding.FragmentMainBinding
import de.tierwohlteam.android.sniffingdetectorapp.viewmodels.MainViewModel
import kotlinx.coroutines.launch

class MainFragment : Fragment() {

    private var _binding: FragmentMainBinding? = null
    private val binding get() = _binding!!

    private val viewModel: MainViewModel by activityViewModels()

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentMainBinding.inflate(inflater, container, false)
        val view = binding.root
        return view
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        binding.btnConnectBT.setOnClickListener {
            val sniffingDetector = SniffingDetector
            connectDialog(this.requireContext(), sniffingDetector)
        }

        lifecycleScope.launchWhenStarted {
            viewModel.sniffingDetector.collect { sniffingStatus ->
                when (sniffingStatus) {
                    SniffingDetectorStatus.SNIFFING -> {
                        binding.message.text = getString(R.string.sniffing)
                        binding.message.setBackgroundColor(Color.GREEN)
                    }
                    SniffingDetectorStatus.NOT_SNIFFING -> {
                        binding.message.text = getString(R.string.not_sniffing)
                        binding.message.setBackgroundColor(Color.BLUE)
                    }
                    else -> { /* NO-OP */
                    }
                }
            }
        }
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }

    private fun connectDialog(context: Context, tool: BTTool) {
        val pairedDevices = tool.getPairedDevices()
        var selectedDevice: BluetoothDevice? = pairedDevices.firstOrNull()
        lifecycleScope.launch {
            tool.connectionMessage.collect {
                Snackbar.make(requireView(), it, Snackbar.LENGTH_LONG).show()
            }
        }
        MaterialAlertDialogBuilder(context)
            .setTitle(resources.getString(R.string.paired))
            .setNeutralButton(resources.getString(R.string.cancel)) { dialog, which ->

            }
            .setPositiveButton(resources.getString(R.string.ok)) { dialog, which ->
                selectedDevice?.let {
                    lifecycleScope.launch {
                        tool.startCommunication(it)
                    }
                }
            }
            .setSingleChoiceItems(pairedDevices.map { it.name } .toTypedArray(), 0) { dialog, which ->
                selectedDevice = pairedDevices[which]
            }
            .show()
    }
}