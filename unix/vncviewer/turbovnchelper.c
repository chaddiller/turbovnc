/*  Copyright (C)2015 D. R. Commander.  All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 *  USA.
 */

#include <string.h>
#include <dlfcn.h>
#include <unistd.h>
#include "jawt_md.h"
#include <X11/extensions/XInput.h>
#include "com_turbovnc_vncviewer_Viewport.h"
#include <X11/Xmd.h>
#include "rfbproto.h"

#ifndef IsXExtensionPointer
#define IsXExtensionPointer 4
#endif


/*
 * These functions are borrowed from OpenSUSE.  They give the window manager a
 * hint that the window is full-screen so the taskbar won't appear on top
 * of it.
 */

#define _NET_WM_STATE_REMOVE        0    /* remove/unset property */
#define _NET_WM_STATE_ADD           1    /* add/set property */

static void netwm_set_state(Display *dpy, Window win, int operation,
                            Atom state)
{
  XEvent e;
  Atom _NET_WM_STATE = XInternAtom(dpy, "_NET_WM_STATE", False);

  memset(&e, 0, sizeof(e));
  e.xclient.type = ClientMessage;
  e.xclient.message_type = _NET_WM_STATE;
  e.xclient.display = dpy;
  e.xclient.window = win;
  e.xclient.format = 32;
  e.xclient.data.l[0] = operation;
  e.xclient.data.l[1] = state;

  XSendEvent(dpy, DefaultRootWindow(dpy), False, SubstructureRedirectMask, &e);
}

static void netwm_fullscreen(Display *dpy, Window win, int state)
{
  Atom _NET_WM_STATE_FULLSCREEN =
    XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
  int op = state ? _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE;
  netwm_set_state(dpy, win, op, _NET_WM_STATE_FULLSCREEN);
}


#define _throw(msg) {  \
  jclass _exccls=(*env)->FindClass(env, "java/lang/Exception");  \
  if (!_exccls) goto bailout;  \
  (*env)->ThrowNew(env, _exccls, msg);  \
  goto bailout;  \
}

#define bailif0(f) {  \
  if(!(f) || (*env)->ExceptionCheck(env)) {  \
    goto bailout;  \
}}

typedef jboolean JNICALL (*__JAWT_GetAWT_type)(JNIEnv* env, JAWT* awt);
static __JAWT_GetAWT_type __JAWT_GetAWT = NULL;

static void *handle = NULL;


JNIEXPORT void JNICALL Java_com_turbovnc_vncviewer_Viewport_x11FullScreen
  (JNIEnv *env, jobject obj, jboolean on)
{
  JAWT awt;
  JAWT_DrawingSurface *ds = NULL;
  JAWT_DrawingSurfaceInfo *dsi = NULL;
  JAWT_X11DrawingSurfaceInfo *x11dsi = NULL;
  jfieldID fid;
  jclass cls;

  awt.version = JAWT_VERSION_1_3;
  if (!handle) {
    if ((handle = dlopen("libjawt.so", RTLD_LAZY)) == NULL)
      _throw(dlerror());
    if ((__JAWT_GetAWT =
         (__JAWT_GetAWT_type)dlsym(handle, "JAWT_GetAWT")) == NULL)
      _throw(dlerror());
  }

  if (__JAWT_GetAWT(env, &awt) == JNI_FALSE)
    _throw("Could not initialize AWT native interface");

  if ((ds = awt.GetDrawingSurface(env, obj)) == NULL)
    _throw("Could not get drawing surface");

  if ((ds->Lock(ds) & JAWT_LOCK_ERROR) != 0)
    _throw("Could not lock surface");

  if ((dsi = ds->GetDrawingSurfaceInfo(ds)) == NULL)
    _throw("Could not get drawing surface info");

  if ((x11dsi = (JAWT_X11DrawingSurfaceInfo*)dsi->platformInfo) == NULL)
    _throw("Could not get X11 drawing surface info");

  netwm_fullscreen(x11dsi->display, x11dsi->drawable, on);
  XSync(x11dsi->display, False);

  if ((cls = (*env)->GetObjectClass(env, obj)) == NULL ||
      (fid = (*env)->GetFieldID(env, cls, "x11win", "J")) == 0)
    _throw("Could not store X window handle");
  (*env)->SetLongField(env, obj, fid, x11dsi->drawable);

  printf("TurboVNC Helper: %s X11 full-screen mode for window 0x%.8lx\n",
         on ? "Enabling" : "Disabling", x11dsi->drawable);

  bailout:
  if (ds) {
    if (dsi) ds->FreeDrawingSurfaceInfo(dsi);
    ds->Unlock(ds);
    awt.FreeDrawingSurface(ds);
  }
}


JNIEXPORT void JNICALL Java_com_turbovnc_vncviewer_Viewport_grabKeyboard
  (JNIEnv *env, jobject obj, jboolean on, jboolean pointer)
{
  JAWT awt;
  JAWT_DrawingSurface *ds = NULL;
  JAWT_DrawingSurfaceInfo *dsi = NULL;
  JAWT_X11DrawingSurfaceInfo *x11dsi = NULL;
  int ret;

  awt.version = JAWT_VERSION_1_3;
  if (!handle) {
    if ((handle = dlopen("libjawt.so", RTLD_LAZY)) == NULL)
      _throw(dlerror());
    if ((__JAWT_GetAWT =
         (__JAWT_GetAWT_type)dlsym(handle, "JAWT_GetAWT")) == NULL)
      _throw(dlerror());
  }

  if (__JAWT_GetAWT(env, &awt) == JNI_FALSE)
    _throw("Could not initialize AWT native interface");

  if ((ds = awt.GetDrawingSurface(env, obj)) == NULL)
    _throw("Could not get drawing surface");

  if ((ds->Lock(ds) & JAWT_LOCK_ERROR) != 0)
    _throw("Could not lock surface");

  if ((dsi = ds->GetDrawingSurfaceInfo(ds)) == NULL)
    _throw("Could not get drawing surface info");

  if ((x11dsi = (JAWT_X11DrawingSurfaceInfo*)dsi->platformInfo) == NULL)
    _throw("Could not get X11 drawing surface info");

  XSync(x11dsi->display, False);
  if (on) {
    int count = 5;
    while ((ret = XGrabKeyboard(x11dsi->display, x11dsi->drawable, True,
                                GrabModeAsync, GrabModeAsync, CurrentTime))
           != GrabSuccess) {
      switch (ret) {
        case AlreadyGrabbed:
          _throw("Could not grab keyboard: already grabbed by another application");
        case GrabInvalidTime:
          _throw("Could not grab keyboard: invalid time");
        case GrabNotViewable:
          /* The window should theoretically be viewable by now, but in
             practice, sometimes a race condition occurs with Swing.  It is
             unclear why, since everything should be happening in the EDT. */
          if (count == 0)
            _throw("Could not grab keyboard: window not viewable");
          usleep(100000);
          count--;
          continue;
        case GrabFrozen:
          _throw("Could not grab keyboard: keyboard frozen by another application");
      }
    }

    if (pointer) {
      ret = XGrabPointer(x11dsi->display, x11dsi->drawable, True,
                         ButtonPressMask | ButtonReleaseMask |
                         ButtonMotionMask | PointerMotionMask, GrabModeAsync,
                         GrabModeAsync, None, None, CurrentTime);
      switch (ret) {
        case AlreadyGrabbed:
          _throw("Could not grab pointer: already grabbed by another application");
        case GrabInvalidTime:
          _throw("Could not grab pointer: invalid time");
        case GrabNotViewable:
          _throw("Could not grab pointer: window not viewable");
        case GrabFrozen:
          _throw("Could not grab pointer: pointer frozen by another application");
      }
    }

    printf("TurboVNC Helper: Grabbed keyboard%s for window 0x%.8lx\n",
           pointer? " & pointer" : "", x11dsi->drawable);
  } else {
    XUngrabKeyboard(x11dsi->display, CurrentTime);
    if (pointer)
      XUngrabPointer(x11dsi->display, CurrentTime);
    printf("TurboVNC Helper: Ungrabbed keyboard%s\n",
           pointer ? " & pointer" : "");
  }
  XSync(x11dsi->display, False);

  bailout:
  if (ds) {
    if (dsi) ds->FreeDrawingSurfaceInfo(dsi);
    ds->Unlock(ds);
    awt.FreeDrawingSurface(ds);
  }
}


#define SET_STRING(cls, obj, fieldName, string) {  \
  jstring str;  \
  bailif0(fid = (*env)->GetFieldID(env, cls, #fieldName,  \
                                   "Ljava/lang/String;"));  \
  bailif0(str = (*env)->NewStringUTF(env, string));  \
  (*env)->SetObjectField(env, obj, fid, (jobject)str);  \
}

#define SET_LONG(cls, obj, fieldName, val) {  \
  bailif0(fid = (*env)->GetFieldID(env, cls, #fieldName, "J"));  \
  (*env)->SetLongField(env, obj, fid, val);  \
}

#define SET_INT(cls, obj, fieldName, val) {  \
  bailif0(fid = (*env)->GetFieldID(env, cls, #fieldName, "I"));  \
  (*env)->SetIntField(env, obj, fid, val);  \
}

#define SET_BOOL(cls, obj, fieldName, val) {  \
  bailif0(fid = (*env)->GetFieldID(env, cls, #fieldName, "Z"));  \
  (*env)->SetBooleanField(env, obj, fid, val);  \
}


JNIEXPORT void JNICALL Java_com_turbovnc_vncviewer_Viewport_extInputEventLoop
  (JNIEnv *env, jobject obj)
{
  jclass cls, eidcls;
  jfieldID fid;
  jmethodID mid;
  Display *dpy = NULL;
  Window win = 0;
  XDeviceInfo *devInfo = NULL;
  XDevice *device = NULL;
  int nDevices = 0, i, ci, ai, nEvents = 0;
  int buttonPressType = -1, buttonReleaseType = -1, motionType = -1;
  XEventClass events[100] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  jobject extInputDevice;

  if ((dpy = XOpenDisplay(NULL)) == NULL)
    _throw("Could not open X display");

  bailif0(cls = (*env)->GetObjectClass(env, obj));
  bailif0(fid = (*env)->GetFieldID(env, cls, "x11win", "J"));
  if ((win = (Window)(*env)->GetLongField(env, obj, fid)) == 0)
    _throw("X window handle has not been initialized");

  printf("TurboVNC Helper: Listening for XInput events on %s (window 0x%.8x)\n",
         DisplayString(dpy), (unsigned int)win);

  if ((devInfo = XListInputDevices(dpy, &nDevices)) == NULL)
    _throw("Could not list XI devices");

  for (i = 0; i < nDevices; i++) {
    char *type;
    XAnyClassPtr classInfo = devInfo[i].inputclassinfo;
    CARD32 canGenerate = 0;

    if (devInfo[i].use != IsXExtensionPointer)
      continue;
    if (devInfo[i].type == None)
      continue;
    type = XGetAtomName(dpy, devInfo[i].type);
    if (!strcmp(type, "MOUSE"))  {
      XFree(type);
      continue;
    }
    XFree(type);

    bailif0(eidcls =
            (*env)->FindClass(env, "com/turbovnc/rfb/ExtInputDevice"));
    bailif0(extInputDevice = (*env)->AllocObject(env, eidcls));

    SET_STRING(eidcls, extInputDevice, name, devInfo[i].name);
    SET_LONG(eidcls, extInputDevice, vendorID, 4242);
    SET_LONG(eidcls, extInputDevice, productID, 4242);
    SET_LONG(eidcls, extInputDevice, id, devInfo[i].id);

    for (ci = 0; ci < devInfo[i].num_classes; ci++) {

      switch (classInfo->class) {

        case ButtonClass:
        {
          XButtonInfoPtr b = (XButtonInfoPtr)classInfo;
          SET_INT(eidcls, extInputDevice, numButtons, b->num_buttons);
          canGenerate |= rfbGIIButtonPressMask | rfbGIIButtonReleaseMask;
          break;
        }

        case ValuatorClass:
        {
          XValuatorInfoPtr v = (XValuatorInfoPtr)classInfo;
          jclass valcls;

          bailif0(valcls = (*env)->FindClass(env,
                  "com/turbovnc/rfb/ExtInputDevice$Valuator"));

          if (v->mode == Absolute)
            canGenerate |= rfbGIIValuatorAbsoluteMask;
          else if (v->mode == Relative)
            canGenerate |= rfbGIIValuatorRelativeMask;

          for (ai = 0; ai < v->num_axes; ai++) {
            jobject valuator;
            XAxisInfoPtr a = &v->axes[ai];
            char longName[75], shortName[5];

            bailif0(valuator = (*env)->AllocObject(env, valcls));
            SET_INT(valcls, valuator, index, ai);
            snprintf(longName, 75, "Valuator %d", ai);
            SET_STRING(valcls, valuator, longName, longName);
            snprintf(shortName, 75, "%d", ai);
            SET_STRING(valcls, valuator, shortName, shortName);
            SET_INT(valcls, valuator, rangeMin, a->min_value);
            SET_INT(valcls, valuator, rangeCenter,
                    (a->min_value + a->max_value) / 2);
            SET_INT(valcls, valuator, rangeMax, a->max_value);
            SET_INT(valcls, valuator, siUnit, 3);
                                           /* ^^ rfbGIIUnitLength, in meters */
            SET_INT(valcls, valuator, siDiv, a->resolution);

            bailif0(mid = (*env)->GetMethodID(env, eidcls, "addValuator",
                    "(Lcom/turbovnc/rfb/ExtInputDevice$Valuator;)V"));
            (*env)->CallVoidMethod(env, extInputDevice, mid, valuator);
          }
          break;
        }
      }
      classInfo = (XAnyClassPtr)((char *)classInfo + classInfo->length);
    }

    SET_LONG(eidcls, extInputDevice, canGenerate, canGenerate);
    if (canGenerate & rfbGIIValuatorAbsoluteMask)
      SET_BOOL(eidcls, extInputDevice, absolute, 1);

    if ((device = XOpenDevice(dpy, devInfo[i].id)) == NULL)
      _throw("Could not open XI device");

    for (ci = 0; ci < device->num_classes; ci++) {
      if (device->classes[ci].input_class == ButtonClass) {
        DeviceButtonPress(device, buttonPressType, events[nEvents]);
        nEvents++;
        DeviceButtonRelease(device, buttonReleaseType, events[nEvents]);
        nEvents++;
      } else if (device->classes[ci].input_class == ValuatorClass) {
        DeviceMotionNotify(device, motionType, events[nEvents]);
        nEvents++;
      }
    }
    XCloseDevice(dpy, device);  device=NULL;

    bailif0(mid = (*env)->GetMethodID(env, cls, "addInputDevice",
            "(Lcom/turbovnc/rfb/ExtInputDevice;)V"));
    (*env)->CallVoidMethod(env, obj, mid, extInputDevice);
  }

  XFreeDeviceList(devInfo);  devInfo = NULL;
  if (nEvents == 0) {
    printf("No extended input devices.\n");
    goto bailout;
  }

  if (XSelectExtensionEvent(dpy, win, events, nEvents))
    _throw("Could not select XI events");

  while (1) {
    int threadRunning = 1;
    XEvent e;

    bailif0(fid = (*env)->GetFieldID(env, cls, "threadRunning", "Z"));
    threadRunning = (*env)->GetBooleanField(env, obj, fid);
    if (!threadRunning) break;

    while (XPending(dpy) > 0) {
      XNextEvent(dpy, &e);

      if (e.type == motionType) {

        XDeviceMotionEvent *motion = (XDeviceMotionEvent *)&e;
        jclass eventcls;  jobject event, jvaluators;
        jint valuators[6];

        bailif0(eventcls =
                (*env)->FindClass(env, "com/turbovnc/rfb/ExtInputEvent"));
        bailif0(mid = (*env)->GetMethodID(env, eventcls, "<init>", "()V"));
        bailif0(event = (*env)->NewObject(env, eventcls, mid));
        SET_INT(eventcls, event, type, rfbGIIValuatorRelative);
        SET_LONG(eventcls, event, deviceID, motion->deviceid);
        SET_LONG(eventcls, event, buttonMask, motion->state);
        SET_INT(eventcls, event, numValuators, motion->axes_count);
        SET_INT(eventcls, event, firstValuator, motion->first_axis);
        SET_INT(eventcls, event, x, motion->x);
        SET_INT(eventcls, event, y, motion->y);

        bailif0(fid = (*env)->GetFieldID(env, eventcls, "valuators", "[I"));
        bailif0(jvaluators =
                (jintArray)(*env)->GetObjectField(env, event, fid));
        for (i = 0; i < motion->axes_count; i++)
          valuators[i] = motion->axis_data[i];
        (*env)->SetIntArrayRegion(env, jvaluators, 0, motion->axes_count,
          valuators);

        bailif0(mid = (*env)->GetMethodID(env, cls, "sendInputEvent",
                "(Lcom/turbovnc/rfb/ExtInputEvent;)V"));
        (*env)->CallVoidMethod(env, obj, mid, event);

      } else if (e.type == buttonPressType || e.type == buttonReleaseType) {

        XDeviceButtonEvent *button = (XDeviceButtonEvent *)&e;
        jclass eventcls;  jobject event, jvaluators;
        jint valuators[6];

        bailif0(eventcls =
                (*env)->FindClass(env, "com/turbovnc/rfb/ExtInputEvent"));
        bailif0(mid = (*env)->GetMethodID(env, eventcls, "<init>", "()V"));
        bailif0(event = (*env)->NewObject(env, eventcls, mid));
        SET_INT(eventcls, event, type, e.type == buttonPressType ?
                rfbGIIButtonPress : rfbGIIButtonRelease);
        SET_LONG(eventcls, event, deviceID, button->deviceid);
        SET_LONG(eventcls, event, buttonMask, button->state);
        SET_INT(eventcls, event, numValuators, button->axes_count);
        SET_INT(eventcls, event, firstValuator, button->first_axis);
        SET_INT(eventcls, event, buttonNumber, button->button);
        bailif0(fid = (*env)->GetFieldID(env, eventcls, "valuators", "[I"));
        bailif0(jvaluators =
                (jintArray)(*env)->GetObjectField(env, event, fid));
        for (i = 0; i < button->axes_count; i++)
          valuators[i] = button->axis_data[i];
        (*env)->SetIntArrayRegion(env, jvaluators, 0, button->axes_count,
                                  valuators);

        bailif0(mid = (*env)->GetMethodID(env, cls, "sendInputEvent",
          "(Lcom/turbovnc/rfb/ExtInputEvent;)V"));
        (*env)->CallVoidMethod(env, obj, mid, event);

      }
    }
    usleep(25000);
  }

  bailout:
  printf("TurboVNC Helper: Shutting down XInput listener");
  if (dpy) {
    printf(" on display %s", DisplayString(dpy));
    if (win) printf(" (window 0x%.8x)", (unsigned int)win);
    if (device) XCloseDevice(dpy, device);
    XCloseDisplay(dpy);
  }
  printf("\n");
  if (devInfo) XFreeDeviceList(devInfo);
}
