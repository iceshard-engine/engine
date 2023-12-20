
package net.example.simple;

import android.app.Activity;
import android.app.NativeActivity;
import android.widget.TextView;
import android.os.Bundle;

public class MyApp extends Activity
{
    static
    {
        System.loadLibrary("simple");
    }

    public native String stringFromJNI();

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        /* Create a TextView and set its text to "Hello world" */
        TextView  tv = new TextView(this);
        tv.setText(stringFromJNI());
        setContentView(tv);
    }
}
