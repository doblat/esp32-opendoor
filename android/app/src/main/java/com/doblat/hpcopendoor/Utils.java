package com.doblat.hpcopendoor;

import static android.content.Context.MODE_PRIVATE;

import android.content.SharedPreferences;
import android.os.Handler;
import android.os.Looper;
import android.widget.Toast;

import java.io.UnsupportedEncodingException;
import java.math.BigInteger;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

public final class Utils {
    public static String macAddress;
    public static String password;
    public static String getMD5Str(String str) {
        byte[] digest = null;
        try {
            MessageDigest md5 = MessageDigest.getInstance("md5");
            digest = md5.digest(str.getBytes("utf-8"));
        } catch (NoSuchAlgorithmException | UnsupportedEncodingException e) {
            //e.printStackTrace();
        }
//16是表示转换为16进制数
        String md5Str = new BigInteger(1, digest).toString(16);
        return md5Str;
    }

    public static void sendNotice(String info) {
        Handler handler = new Handler(Looper.getMainLooper());
        handler.post(new Runnable() {
            @Override
            public void run() {
                //放在UI线程弹Toast
                Toast.makeText(MainActivity.context, info, Toast.LENGTH_LONG).show();
            }
        });
    }
    public static boolean checkConfig() {
        SharedPreferences config = MainActivity.context.getSharedPreferences("config", MODE_PRIVATE);
        macAddress = config.getString("macAddress", "");
        password = config.getString("password", "");
        if (macAddress.equals("") | password.equals("")) {
            sendNotice("配置信息未填写，请先填写配置信息。");
            return false;
        }
        return true;
    }

    public static String getMacAddress() {
        return macAddress;
    }
    public static String getPassword() {
        return password;
    }
}
