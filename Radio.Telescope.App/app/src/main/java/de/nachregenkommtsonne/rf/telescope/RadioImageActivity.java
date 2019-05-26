package de.nachregenkommtsonne.rf.telescope;

import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.drawable.BitmapDrawable;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.widget.ImageView;
import android.widget.SeekBar;

import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.IntBuffer;
import java.util.zip.GZIPInputStream;

public class RadioImageActivity extends AppCompatActivity {

    FloatBuffer[][] buffers;
    int width;
    int height;
    boolean running = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_radio_image);

        try {

            Bundle extras = getIntent().getExtras();
            String file = extras.getString("filename");

            InputStream fileStream = new FileInputStream("/storage/emulated/0/" + file);
            InputStream gzipStream = new GZIPInputStream(fileStream);

            byte[] data = new byte[8];
            gzipStream.read(data);
            IntBuffer intBuffer = ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN).asIntBuffer();
            width = intBuffer.get(0);
            height = intBuffer.get(1);

            int samples = 1200;

            buffers = new FloatBuffer[width][height];

            for (int x = 0; x < width; x++) {
                for (int y = 0; y < height; y++) {
                    data = new byte[samples * 4];
                    int bytesRead = 0;

                    while (gzipStream.available() != 0 && bytesRead != samples * 4) {
                        bytesRead += gzipStream.read(data, bytesRead, samples * 4 - bytesRead);
                    }

                    FloatBuffer floatBuffer = ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN).asFloatBuffer();
                    buffers[x][y] = floatBuffer;
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
            return;
        }

        final SeekBar seekBarFrequency = findViewById(R.id.seekBarFrequency);
        seekBarFrequency.setMax(1200-1);

        final SeekBar seekBarBandWidth = findViewById(R.id.seekBarBandWidth);
        seekBarBandWidth.setMax(200-1);

        final SeekBar seekBarExposure = findViewById(R.id.seekBarExposure);
        seekBarExposure.setMax(1000-1);

        seekBarFrequency.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                updateImage(progress, seekBarBandWidth.getProgress(), seekBarExposure.getProgress());
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
            }
        });

        seekBarBandWidth.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                updateImage(seekBarFrequency.getProgress(), progress, seekBarExposure.getProgress());
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
            }
        });

        seekBarExposure.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                updateImage(seekBarFrequency.getProgress(), seekBarBandWidth.getProgress(), progress);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
            }
        });
    }

    private void updateImage(int progress, int progress2, int progress3){
        if (running)
            return;

        running = true;

        ImageView imageView = findViewById(R.id.imageView);

        final Bitmap bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        bitmap.eraseColor(Color.GREEN);

        Thread[] threads = new Thread[width];

        final int frequency = progress;
        final int bandwidth = progress2 + 1;
        final int exposure = progress3;

        for (int i = 0; i < width; i++) {
            final int x = i;
            Thread thread = new Thread(new Runnable() {
                @Override
                public void run() {
                    for (int y = 0; y < height; y++) {
                        int c = calculateColor(buffers[x][y], frequency, bandwidth, exposure);
                        bitmap.setPixel(x, y, c);
                    }
                }
            });

            thread.run();
            threads[i] = thread;
        }

        for (int i = 0; i < width; i++) {
            try {
                threads[i].join();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }

        imageView.setImageDrawable(new BitmapDrawable(getResources(), bitmap));
        running = false;
    }

    private int calculateColor(FloatBuffer floatBuffer, int baseFrequency, int bandwidth, float dimFactor) {

        int redFrequency = baseFrequency + bandwidth / 4;
        int greenFrequency = baseFrequency + bandwidth / 2;
        int blueFrequency = baseFrequency + bandwidth / 2 + bandwidth / 4;

        float red = 0.0f;
        float green = 0.0f;
        float blue = 0.0f;

        for (int frequency = baseFrequency; frequency < baseFrequency + bandwidth; frequency++)
        {
            float affectsRed = 1.0f - Math.min(1.0f, Math.abs(redFrequency - frequency) / (bandwidth / 4.0f));
            float affectsGreen = 1.0f - Math.min(1.0f, Math.abs(greenFrequency - frequency) / (bandwidth / 4.0f));
            float affectsBlue = 1.0f - Math.min(1.0f, Math.abs(blueFrequency - frequency) / (bandwidth / 4.0f));

            float signalStrength = floatBuffer.get(frequency);

            float v = signalStrength + 80.0f;

            if (v < 0.0f)
                continue;

            red += affectsRed * v;
            green += affectsGreen * v;
            blue += affectsBlue * v;
        }

        red = red / dimFactor;
        green = green / dimFactor;
        blue = blue / dimFactor;

        if (red > 1.0f)
            red = 1.0f;

        if (green > 1.0f)
            green = 1.0f;

        if (blue > 1.0f)
            blue = 1.0f;

        return Color.rgb((int)(red * 255), (int)(green * 255), (int)(blue * 255));
    }
}
