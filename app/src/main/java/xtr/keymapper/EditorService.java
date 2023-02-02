package xtr.keymapper;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.IBinder;
import android.os.RemoteException;
import android.view.Window;

import androidx.appcompat.view.ContextThemeWrapper;

import xtr.keymapper.server.InputService;

public class EditorService extends Service implements EditorUI.OnHideListener {
    private EditorUI editor;
    private IRemoteService mService;

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        mService = InputService.getInstance();

        if (editor != null) editor.hideView();

        Context context = new ContextThemeWrapper(this, R.style.Theme_MaterialComponents);
        editor = new EditorUI(context, this);
        editor.open();

        if (mService != null)
            try {
                mService.registerOnKeyEventListener(editor);
            } catch (RemoteException ignored) {
            }

        return super.onStartCommand(intent, flags, startId);
    }

    @Override
    public void onHideView() {
        try {
            mService.unregisterOnKeyEventListener(editor);
        } catch (RemoteException ignored) {
        }
        editor = null;
        stopSelf();
    }

    @Override
    public boolean getEvent() {
        return mService != null;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
}