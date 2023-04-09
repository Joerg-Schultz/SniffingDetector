package de.tierwohlteam.android.sniffingdetectorapp

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import de.tierwohlteam.android.sniffingdetectorapp.fragments.MainFragment

class MainActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
    }
}