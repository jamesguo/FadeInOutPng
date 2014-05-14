package com.example.fadeinoutpng;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;


import android.content.Context;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.os.Bundle;
import android.os.Environment;
import android.support.v4.app.FragmentActivity;
import android.view.Display;
import android.view.View;

public class MainActivity extends FragmentActivity {
	static {
		try {
			java.lang.System.loadLibrary("CustomerPng");
		} catch (Exception e) {
			// TODO: handle exception
		}
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		AssetManager assetManager = getAssets();
		InputStream inputStream = null;
		try {
			inputStream = assetManager.open("ic_launcher-web.png");
			File file = new File(getApplicationContext().getFilesDir().getAbsolutePath() + File.separator + "ic_launcher-web.png");
			OutputStream outputStream = new FileOutputStream(file);
			int bytesRead;
			byte[] buf = new byte[4 * 1024]; // 4K buffer
			while ((bytesRead = inputStream.read(buf)) != -1) {
				outputStream.write(buf, 0, bytesRead);
			}
			outputStream.flush();
			outputStream.close();
			inputStream.close();
			loadImage(file.getAbsolutePath());
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
        setContentView(new PlasmaView(this, 512, 512));
	}

	public static native void loadImage(String filepath);

	public static native void startFade(Bitmap bitmap, long time_ms);
}

class PlasmaView extends View {
	private Bitmap mBitmap;
	private long mStartTime;

	public PlasmaView(Context context, int width, int height) {
		super(context);
		mBitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
		mStartTime = System.currentTimeMillis();
	}

	@Override
	protected void onDraw(Canvas canvas) {
		// canvas.drawColor(0xFFCCCCCC);
		MainActivity.startFade(mBitmap, System.currentTimeMillis() - mStartTime);
		canvas.drawBitmap(mBitmap, 0, 0, null);
		invalidate();
	}
}
