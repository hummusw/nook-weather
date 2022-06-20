# Nook Setup Notes

## How to update the firmware of an unrooted Nook Simple Touch
1. Copy the update file (`nook_1_2_update.zip`) to the root directory of the Nook
2. Wait for the nook to go to sleep
3. Firmware update will automatically start

## How to root a Nook Simple Touch
Instructions from [this MUO article](https://www.makeuseof.com/tag/hack-your-nook-simple-touch-into-a-super-e-reader-in-three-easy-steps/)

1. Unzip and burn the ClockWorkMod image file (`128mb_clockwork-rc2.img`) onto a microSD card
2. After imaging, copy kernel file (`install-kernel-usbhost-176.zip`) to root directory of the microSD card
3. Turn off Nook, insert microSD card, turn on nook
4. Apply update from .zip file on SD card
5. Turn off Nook and remove microSD card
6. Burn NookManager image file (`NookManager.img`) onto microSD card
7. Insert microSD card into Nook and turn it on
8. Select Root my Nook

## How to install Electric Sign app using adb
Instructions from [Android developers documentation](https://developer.android.com/studio/command-line/adb#move)
1. Connect the rooted Nook to the computer via USB
2. Start adb in the same directory as the Electric Sign apk file (`ElectricSign_v1.0.3.apk`)
3. Use the adb install command as follows:

 ```adb install ElectricSign_v1.0.3.apk```

## How to remove "Cell standby service"
Instructions from [this XDA developers forum thread](https://forum.xda-developers.com/t/script-remove-cell-standby-service-no-effect-on-battery-life.888216/)

Note: there does not seem to be conclusive evidence that this increases battery life, but it does not seem to have any adverse effects.

1. Connect the rooted Nook to the computer via USB
2. Start adb and issue the following commands:


```
adb shell mount -o remount,rw /dev/block/mmcblk0p5 /system
adb shell mv /system/app/Phone.apk /system/app/Phone.OLD`
adb shell mv /system/app/TelephonyProvider.apk /system/app/TelephonyProvider.OLD
adb reboot
```
