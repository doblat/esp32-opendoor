package com.doblat.hpcopendoor;

import androidx.appcompat.app.AppCompatActivity;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import org.w3c.dom.Text;

public class MainActivity extends AppCompatActivity {
    public static Context context;
    public static Activity activity;
//    public static TextView textView;
    public static TextView textView_title;
    public static BLE ble;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        context = this;
        activity = this;
        ble = new BLE();

//        textView = findViewById(R.id.textView);
        textView_title = findViewById(R.id.textView_title);
        Button button = findViewById(R.id.button);
        View.OnClickListener onClickListener = new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Permission.run(context, activity);
                Intent intentSettings = new Intent(activity, SettingsActivity.class);
//                intentSettings.setAction("android.intent.action.SETTINGS");
//                intentSettings.addCategory(Intent.CATEGORY_DEFAULT);
                startActivity(intentSettings);
            }
        };
        button.setOnClickListener(onClickListener);

//        Button button2 = findViewById(R.id.button2);
//        View.OnClickListener onClickListener2 = new View.OnClickListener() {
//            @Override
//            public void onClick(View v) {
//                ble.read(0);
//            }
//        };
//        button2.setOnClickListener(onClickListener2);

        Button button3 = findViewById(R.id.button3);
        View.OnClickListener onClickListener3 = new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                ble.disconnect();
            }
        };
        button3.setOnClickListener(onClickListener3);

        Button button_open = findViewById(R.id.button_open);
        View.OnClickListener onClickListener_open = new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if(Utils.checkConfig()) {
                    ble.run();
                }
            }
        };
        button_open.setOnClickListener(onClickListener_open);

        Button button_exit = findViewById(R.id.button_exit);
        View.OnClickListener onClickListener_exit = new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                finish(); //结束程序
                System.exit(0);  //退出程序
            }
        };
        button_exit.setOnClickListener(onClickListener_exit);

        if(Utils.checkConfig()) {
            ble.run();
        } else {
            textView_title.setText("未配置");
        }
    }


}