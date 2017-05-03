package com.test.cppframebufferobject;

import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.PixelFormat;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class MainActivity extends Activity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

//        SurfaceView surface = ((SurfaceView)findViewById(R.id.surface));
        SurfaceView surface = new SurfaceView(this);
        setContentView(surface);
//        surface.setBackgroundColor(0x80808030);

        /* 透過設定 */
        surface.getHolder().setFormat(PixelFormat.TRANSLUCENT);
        surface.setZOrderOnTop(true);

        /* コールバック設定 */
        surface.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                /* C++ */
                NativeFunc.create(0);
                NativeFunc.surfaceCreated(0, holder.getSurface());
            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                final Bitmap image = BitmapFactory.decodeResource(getResources(), R.drawable.images);  // 画像オブジェクトを作成する
                if (image == null)
                    throw new RuntimeException("画像オブジェクトの作成に失敗");
                NativeFunc.surfaceChanged(0, width, height);
            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {
                NativeFunc.surfaceDestroyed(0);
            }
        });

    }
}
