package com.doblat.hpcopendoor;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanResult;
import android.content.pm.PackageManager;
import android.os.Handler;


import androidx.core.app.ActivityCompat;


import java.util.UUID;

public final class BLE {

    private boolean isScanning = false;
    private Handler handler = new Handler();
    private static final long SCAN_PERIOD = 2_000; //扫描时间
    String deviceAddress = null;
    boolean serviceDiscovered = false;
    int writeCharacteristicCode = 0;

    public void run() {
        if (!isBluetoothEnabled()) {
            return;
        }
        if (serviceDiscovered) {
            read(0);
            return;
        }
        if (deviceAddress == null) {
            startScan();
        } else {
            connect(deviceAddress);
        }
    }

    private BluetoothAdapter getBluetoothAdapter() {
        //        BluetoothManager bluetoothManager = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
        //        BluetoothAdapter bluetoothAdapter = bluetoothManager.getAdapter();
        //        return bluetoothAdapter;

        // 等价于如下
        return BluetoothAdapter.getDefaultAdapter();
    }

    private boolean isBluetoothEnabled() {
        boolean isEnabled = getBluetoothAdapter().isEnabled();
        if(!isEnabled) {
            MainActivity.textView_title.setText("蓝牙未开启");
            Utils.sendNotice("蓝牙未启用，请打开蓝牙。");
        }
        return isEnabled;
    }

    /**
     * 开始扫描蓝牙设备
     */
    private void startScan() {
        if (ActivityCompat.checkSelfPermission(MainActivity.context, android.Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED) {
            // TODO: Consider calling
            //    ActivityCompat#requestPermissions
            // here to request the missing permissions, and then overriding
            //   public void onRequestPermissionsResult(int requestCode, String[] permissions,
            //                                          int[] grantResults)
            // to handle the case where the user grants the permission. See the documentation
            // for ActivityCompat#requestPermissions for more details.
            return;
        }
        if (!isScanning) {
            //Log.e("TAG", "开始扫描");
//            MainActivity.textView.setText("开始扫描");
            MainActivity.textView_title.setText("正在扫描");
            isScanning = true;
            BluetoothLeScanner bluetoothLeScanner = getBluetoothAdapter().getBluetoothLeScanner();
            handler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    isScanning = false;
                    if (ActivityCompat.checkSelfPermission(MainActivity.context, android.Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED) {
                        // TODO: Consider calling
                        //    ActivityCompat#requestPermissions
                        // here to request the missing permissions, and then overriding
                        //   public void onRequestPermissionsResult(int requestCode, String[] permissions,
                        //                                          int[] grantResults)
                        // to handle the case where the user grants the permission. See the documentation
                        // for ActivityCompat#requestPermissions for more details.
                        return;
                    }
//                    MainActivity.textView.setText("扫描停止");
                    MainActivity.textView_title.setText("未找到设备");
                    bluetoothLeScanner.stopScan(leScanCallback);
                }
            }, SCAN_PERIOD);
            bluetoothLeScanner.startScan(leScanCallback);
        }
    }

    /**
     * 停止扫描蓝牙设备
     */
    public void stopScan() {
        if (ActivityCompat.checkSelfPermission(MainActivity.context, android.Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED) {
            // TODO: Consider calling
            //    ActivityCompat#requestPermissions
            // here to request the missing permissions, and then overriding
            //   public void onRequestPermissionsResult(int requestCode, String[] permissions,
            //                                          int[] grantResults)
            // to handle the case where the user grants the permission. See the documentation
            // for ActivityCompat#requestPermissions for more details.
            return;
        }
        if (isScanning) {
            //Log.e("TAG", "停止扫描");
            isScanning = false;
            handler.removeCallbacksAndMessages(null);
            BluetoothLeScanner bluetoothLeScanner = getBluetoothAdapter().getBluetoothLeScanner();
            bluetoothLeScanner.stopScan(leScanCallback);
        }
    }

    /**
     * 扫描结果回调
     */
    private final ScanCallback leScanCallback = new ScanCallback() {

        @Override
        public void onScanResult(int callbackType, ScanResult result) {
            super.onScanResult(callbackType, result);
            BluetoothDevice device = result.getDevice();
            if (device != null) {
                if (ActivityCompat.checkSelfPermission(MainActivity.context, android.Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                    // TODO: Consider calling
                    //    ActivityCompat#requestPermissions
                    // here to request the missing permissions, and then overriding
                    //   public void onRequestPermissionsResult(int requestCode, String[] permissions,
                    //                                          int[] grantResults)
                    // to handle the case where the user grants the permission. See the documentation
                    // for ActivityCompat#requestPermissions for more details.
                    return;
                }
//                String deviceInfoStr = "\n" +
//                        "设备名：" + device.getName() + "\n" +
//                        "地址：" + device.getAddress() + "\n" +
//                        "uuids：" + Arrays.toString(device.getUuids());
                if (filterDevice(device.getAddress())) {
                    //Log.e("TAG", "找到指定设备，停止扫描");
                    stopScan();
                    //deviceType = getDeviceType(device.getName());
                    deviceAddress = device.getAddress();
                    //viewBinding.tvDevice.setText(deviceInfoStr);
                    //MainActivity.textView.setText(deviceInfoStr);
                    connect(deviceAddress);
                }

            }
        }
    };

    private boolean filterDevice(String deviceMac) {
        return Utils.getMacAddress().equals(deviceMac);
    }

    BluetoothGatt mBluetoothGatt;

    public void connect(final String address) {
        if (ActivityCompat.checkSelfPermission(MainActivity.context, android.Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
            // TODO: Consider calling
            //    ActivityCompat#requestPermissions
            // here to request the missing permissions, and then overriding
            //   public void onRequestPermissionsResult(int requestCode, String[] permissions,
            //                                          int[] grantResults)
            // to handle the case where the user grants the permission. See the documentation
            // for ActivityCompat#requestPermissions for more details.
            return;
        }
        try {
            BluetoothAdapter bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
            BluetoothDevice device = bluetoothAdapter.getRemoteDevice(address);
            // 连接Gatt服务，用于通信
            mBluetoothGatt = device.connectGatt(MainActivity.context, false, mGattCallback);
//            MainActivity.textView.setText("ConnectGatt");
        } catch (IllegalArgumentException e) {
            //Log.e("TAG", "连接异常");
            MainActivity.textView_title.setText("Gatt连接异常");
        }
    }

    public void disconnect() {
        if (ActivityCompat.checkSelfPermission(MainActivity.context, android.Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
            // TODO: Consider calling
            //    ActivityCompat#requestPermissions
            // here to request the missing permissions, and then overriding
            //   public void onRequestPermissionsResult(int requestCode, String[] permissions,
            //                                          int[] grantResults)
            // to handle the case where the user grants the permission. See the documentation
            // for ActivityCompat#requestPermissions for more details.
            return;
        }
        if(mBluetoothGatt!=null) {
            mBluetoothGatt.disconnect();
        }
        //mBluetoothGatt.close();
    }

    private BluetoothGatt gatt_global;
    private UUID serviceUuid = UUID.fromString("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
    private UUID characteristicUuid = UUID.fromString("0410BB38-F577-9276-A9FC-47F94B9646ED");
    private UUID characteristicWriteUuid = UUID.fromString("beb5483e-36e1-4688-b7f5-ea07361b26a8");
    private UUID characteristicWebserverWriteUuid = UUID.fromString("9e641724-2638-4250-9955-ce0576bd7bd0");

    // 定义连接回调
    private final BluetoothGattCallback mGattCallback = new BluetoothGattCallback() {
        /**
         * 连接状态监听
         */
        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            if (ActivityCompat.checkSelfPermission(MainActivity.context, android.Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                // TODO: Consider calling
                //    ActivityCompat#requestPermissions
                // here to request the missing permissions, and then overriding
                //   public void onRequestPermissionsResult(int requestCode, String[] permissions,
                //                                          int[] grantResults)
                // to handle the case where the user grants the permission. See the documentation
                // for ActivityCompat#requestPermissions for more details.
                return;
            }
            gatt_global = gatt;
            super.onConnectionStateChange(gatt, status, newState);
            //Log.e("TAG", "onConnectionStateChange");
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                // 成功连接Gatt服务
                //Log.e("TAG", "成功连接Gatt服务");
                //connectionState = STATE_CONNECTED;
                //broadcastUpdate(ACTION_GATT_CONNECTED);
                // 发现BLE提供的服务
                mBluetoothGatt.discoverServices();
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                // 与GATT服务断开连接2
                //Log.e("TAG", "与GATT服务断开连接");
                //connectionState = STATE_DISCONNECTED;
                //broadcastUpdate(ACTION_GATT_DISCONNECTED);
                serviceDiscovered = false;
//                MainActivity.textView.setText("DISCONNECTED");
                MainActivity.textView_title.setText("连接已断开");
                mBluetoothGatt.close();
                mBluetoothGatt = null;
                deviceAddress = null;
            }
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                // 获取需要的特征值
//                MainActivity.textView.setText("ServiceDiscovered");
                MainActivity.textView_title.setText("连接成功");
                serviceDiscovered = true;
//                BluetoothGattCharacteristic characteristic = gatt.getService(serviceUuid)
//                        .getCharacteristic(characteristicUuid);
//                if (ActivityCompat.checkSelfPermission(MainActivity.context, android.Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
//                    // TODO: Consider calling
//                    //    ActivityCompat#requestPermissions
//                    // here to request the missing permissions, and then overriding
//                    //   public void onRequestPermissionsResult(int requestCode, String[] permissions,
//                    //                                          int[] grantResults)
//                    // to handle the case where the user grants the permission. See the documentation
//                    // for ActivityCompat#requestPermissions for more details.
//                    return;
//                }
                //gatt.readCharacteristic(characteristic);
            } else {
                // 获取特征值失败
            }
        }

        @Override
        public void onCharacteristicRead(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
//                MainActivity.textView.setText("READ");
                // 读取成功，处理返回的数据
                byte[] data = characteristic.getValue();
                String value = new String(data);
                //Integer value_int = Integer.parseInt(value, 16);
//                MainActivity.textView.setText(value);
                // 处理数据

                //LIGHT
                writeCharacteristic(value);

            } else {
                // 读取失败
            }
        }
    };

//    public String bytesToHexString(byte[] src) {
//        StringBuilder stringBuilder = new StringBuilder("");
//        if (src == null || src.length <= 0) {
//            return null;
//        }
//        for (int i = 0; i < src.length; i++) {
//            int v = src[i] & 0xFF;
//            String hv = Integer.toHexString(v);
//            if (hv.length() < 2) {
//                stringBuilder.append(0);
//            }
//            stringBuilder.append(hv);
//        }
//        return stringBuilder.toString();
//    }


    public boolean read(int writeCharacteristicCode) {
        //MainActivity.textView.setText("ServiceDiscovered");
        if (!isBluetoothEnabled()) {
            return false;
        }
        if (!serviceDiscovered){
            Utils.sendNotice("未连接，请等待连接成功后再发送请求。");
            return false;
        }

        if (ActivityCompat.checkSelfPermission(MainActivity.context, android.Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
            // TODO: Consider calling
            //    ActivityCompat#requestPermissions
            // here to request the missing permissions, and then overriding
            //   public void onRequestPermissionsResult(int requestCode, String[] permissions,
            //                                          int[] grantResults)
            // to handle the case where the user grants the permission. See the documentation
            // for ActivityCompat#requestPermissions for more details.
            return false;
        }
        this.writeCharacteristicCode = writeCharacteristicCode;
        BluetoothGattCharacteristic characteristic = gatt_global.getService(serviceUuid)
                .getCharacteristic(characteristicUuid);
        gatt_global.readCharacteristic(characteristic);
        return true;
    }

    public void writeCharacteristic(String salt) {
        if (mBluetoothGatt != null) {
            BluetoothGattCharacteristic characteristic;
            if (writeCharacteristicCode == 1) {
                characteristic = gatt_global.getService(serviceUuid)
                        .getCharacteristic(characteristicWebserverWriteUuid);
            } else {
                characteristic = gatt_global.getService(serviceUuid)
                        .getCharacteristic(characteristicWriteUuid);
            }

            characteristic.setValue(Utils.getMD5Str(salt+Utils.getPassword()));
            if (ActivityCompat.checkSelfPermission(MainActivity.context, android.Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                // TODO: Consider calling
                //    ActivityCompat#requestPermissions
                // here to request the missing permissions, and then overriding
                //   public void onRequestPermissionsResult(int requestCode, String[] permissions,
                //                                          int[] grantResults)
                // to handle the case where the user grants the permission. See the documentation
                // for ActivityCompat#requestPermissions for more details.
                return;
            }
            mBluetoothGatt.writeCharacteristic(characteristic);
            Utils.sendNotice("指令已发出");
        }
    }
}
