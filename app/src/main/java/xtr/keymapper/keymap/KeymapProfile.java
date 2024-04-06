package xtr.keymapper.keymap;

import static xtr.keymapper.dpad.Dpad.MAX_DPADS;

import android.os.Parcel;
import android.os.Parcelable;

import androidx.annotation.NonNull;

import java.util.ArrayList;

import xtr.keymapper.dpad.Dpad;
import xtr.keymapper.mouse.MouseAimConfig;
import xtr.keymapper.swipekey.SwipeKey;

public class KeymapProfile implements Parcelable {
    public String packageName = "xtr.keymapper";
    public Dpad[] dpadArray = new Dpad[MAX_DPADS];
    public MouseAimConfig mouseAimConfig = null;
    public ArrayList<KeymapProfileKey> keys = new ArrayList<>();
    public ArrayList<SwipeKey> swipeKeys = new ArrayList<>();
    public KeymapProfileKey rightClick;
    public boolean disabled = false;

    public KeymapProfile() {

    }

    protected KeymapProfile(Parcel in) {
        packageName = in.readString();
        dpadArray = in.createTypedArray(Dpad.CREATOR);
        mouseAimConfig = in.readParcelable(MouseAimConfig.class.getClassLoader());
        keys = in.createTypedArrayList(KeymapProfileKey.CREATOR);
        swipeKeys = in.createTypedArrayList(SwipeKey.CREATOR);
        rightClick = in.readParcelable(KeymapProfileKey.class.getClassLoader());
        disabled = in.readByte() != 0;
    }

    public static final Creator<KeymapProfile> CREATOR = new Creator<>() {
        @Override
        public KeymapProfile createFromParcel(Parcel in) {
            return new KeymapProfile(in);
        }

        @Override
        public KeymapProfile[] newArray(int size) {
            return new KeymapProfile[size];
        }
    };

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(@NonNull Parcel dest, int flags) {
        dest.writeString(packageName);
        dest.writeTypedArray(dpadArray, flags);
        dest.writeParcelable(mouseAimConfig, flags);
        dest.writeTypedList(keys);
        dest.writeTypedList(swipeKeys);
        dest.writeParcelable(rightClick, flags);
        dest.writeByte((byte) (disabled ? 1 : 0));
    }
}
