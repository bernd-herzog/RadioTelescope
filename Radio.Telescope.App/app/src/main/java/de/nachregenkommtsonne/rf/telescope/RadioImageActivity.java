package de.nachregenkommtsonne.rf.telescope;

import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.View;
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

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_radio_image);
        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        FloatingActionButton fab = findViewById(R.id.fab);
        fab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Snackbar.make(view, "Replace with your own action", Snackbar.LENGTH_LONG)
                        .setAction("Action", null).show();
            }
        });

        getSupportActionBar().setDisplayHomeAsUpEnabled(true);

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

//                    Snackbar.make(view, "Reading " + width + "x" + height +  " ...", Snackbar.LENGTH_LONG)
            //                          .setAction("Action", null).show();
        } catch (IOException e) {
            e.printStackTrace();
            return;
        }
        //  Snackbar.make(view, "Done reading" + i, Snackbar.LENGTH_LONG).setAction("Action", null).show();





        SeekBar seekBar = (SeekBar) findViewById(R.id.seekBar);
        seekBar.setMax(0);
        seekBar.setMax(1200-1);
        seekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                ImageView imageView = (ImageView) findViewById(R.id.imageView);

                Bitmap bitmap = Bitmap.createBitmap(200, 200, Bitmap.Config.ARGB_8888);
                //bitmap.setPixel(100, 100, Color.GREEN);
                bitmap.eraseColor(Color.GREEN);

                for (int x = 0; x < width; x++) {
                    for (int y = 0; y < height; y++) {
                        float v = buffers[x][y].get(progress);

                        if (v < -80){
                            bitmap.setPixel(x, y, Color.BLUE);
                        } else if (v > -10) {
                            bitmap.setPixel(x, y, Color.RED);
                        } else {
                            float v1 = v + 55 * 4;
                            int color = Math.min(Math.max(0, (int) v1), 255);
                            bitmap.setPixel(x, y, Color.rgb(color, color, color));
                        }
                    }
                }

                imageView.setImageDrawable(new BitmapDrawable(getResources(), bitmap));
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {

            }
        });
    }
}
