package com.doblat.hpcopendoor;

import androidx.appcompat.app.AppCompatActivity;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import org.w3c.dom.Text;

public class SettingsActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_settings);

        Activity activity = this;
        Context context = this;

        SharedPreferences config = getSharedPreferences("config", MODE_PRIVATE);
        String macAddress = config.getString("macAddress", "");
        String password = config.getString("password", "");

        EditText editText_macAddress = findViewById(R.id.editText_macAddress);
        EditText editText_password = findViewById(R.id.editText_password);
        editText_macAddress.setText(macAddress);
        editText_password.setText(password);

        Button button_getPermission = findViewById(R.id.button_getPermission);
        View.OnClickListener onClickListener_getPermission = new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                TextView textView_getPermission = findViewById(R.id.textView_getPermission);
                if(Permission.run(context, activity)) {
                    textView_getPermission.setText("权限配置（已完成授权）");
                } else {
                    textView_getPermission.setText("权限配置（未授权，蓝牙连接需要位置和蓝牙权限，再次点击申请可刷新状态）");
//                    if(Permission.run(context, activity)) {
//                        textView_getPermission.setText("权限配置（已完成授权）");
//                    }
                }
            }
        };
        button_getPermission.setOnClickListener(onClickListener_getPermission);


        Button button_save = findViewById(R.id.button_save);
        View.OnClickListener onClickListener = new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                SharedPreferences.Editor configEditor = config.edit();
                configEditor.putString("macAddress",editText_macAddress.getText().toString());
                configEditor.putString("password",editText_password.getText().toString());
                if(configEditor.commit()) {
                    Utils.sendNotice("保存成功");
                } else {
                    Utils.sendNotice("保存失败");
                }

            }
        };
        button_save.setOnClickListener(onClickListener);

        Button button_runAdminMode = findViewById(R.id.button_runAdminMode);
        View.OnClickListener onClickListener_runAdminMode = new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (MainActivity.ble.read(1)) {
                    String content = "请手动连接到锁热点（HpcSmartLock）" +
                            "\n" + "关闭流量，访问192.168.1.1，进行管理。";
                    AlertDialog alertDialog1 = new AlertDialog.Builder(context)
                            .setTitle("已发送管理模式启动指令")//标题
                            .setMessage(content)//内容
                            .setIcon(R.mipmap.ic_launcher)//图标
                            .create();
                    alertDialog1.show();
                }
            }
        };
        button_runAdminMode.setOnClickListener(onClickListener_runAdminMode);

        Button button_about = findViewById(R.id.button_about);
        View.OnClickListener onClickListener_about = new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String content = "v1.0" +
                        "\n" + "By doblat" +
                        "\n\n" + "如果处于未连接状态，点击一键启动只会执行连接操作，需要等待连接成功后再次点击启动。";
                AlertDialog alertDialog1 = new AlertDialog.Builder(context)
                        .setTitle("HpcOpenDoor")//标题
                        .setMessage(content)//内容
                        .setIcon(R.mipmap.ic_launcher)//图标
                        .create();
                alertDialog1.show();
            }
        };
        button_about.setOnClickListener(onClickListener_about);
    }
}