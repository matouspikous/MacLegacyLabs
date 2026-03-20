/*
 *  MacMiniAudioFix.c  —  v8.0  Working Release
 *
 *  Mac OS 9 System Extension for Mac Mini G4 audio.
 *  Enables the output amplifier GPIOs that the screamer
 *  driver leaves disabled.
 *
 *  Discovered through empirical testing:
 *  - Pin 0x67: headphone detect (bit 1 = 1: no HP, 0: HP in)
 *  - Pin 0x51: also changes with HP (0xFF->0x7F)
 *  - Pins 0x6E, 0x6F, 0x70: amp enables (write 0x05 -> 0x07)
 *  - Pin 0x79: additional amp enable
 *  - All four must be enabled for sound output
 *
 *  Build: CodeWarrior, PPC Code Resource, INIT ID 0
 *  Link: InterfaceLib, NameRegistryLib, DriverServicesLib
 */

#include <Types.h>
#include <Memory.h>
#include <Resources.h>
#include <Gestalt.h>
#include <OSUtils.h>
#include <LowMem.h>
#include <NameRegistry.h>
#include <DriverServices.h>
#include <Icons.h>

static OSStatus FindIOControllerBase(void);
static void     ShowINITIcon(short iconID, Boolean advance);

#define kIconOK_ID      128
#define kIconFail_ID    129
#define kIconSkip_ID    130

static UInt32   gMacIOBase = 0;

/* GPIO pin assignments (Mac Mini G4 Intrepid) */
#define GPIO_HP_DETECT  0x67    /* bit 1: 1=no HP, 0=HP inserted */
#define GPIO_AMP_1      0x6E    /* Amp enable */
#define GPIO_AMP_2      0x6F    /* Amp enable */
#define GPIO_AMP_3      0x70    /* Amp enable */
#define GPIO_AMP_4      0x79    /* Amp enable (master?) */
#define GPIO_AMP_ON     0x05    /* Output enabled + data HIGH */

/* ---- GPIO access ---- */

static UInt8 ReadGPIO(UInt32 offset)
{
    volatile UInt8 *a;
    UInt8 v;
    a = (volatile UInt8 *)(gMacIOBase + offset);
    v = *a;
    SynchronizeIO();
    return v;
}

static void WriteGPIO(UInt32 offset, UInt8 value)
{
    volatile UInt8 *a;
    a = (volatile UInt8 *)(gMacIOBase + offset);
    *a = value;
    SynchronizeIO();
}

/* ---- Find mac-io ---- */

static OSStatus FindIOControllerBase(void)
{
    RegEntryID      entry;
    RegEntryIter    cookie;
    Boolean         done = false;
    OSStatus        err;
    UInt32          regProp[20];
    RegPropertyValueSize sz;

    err = RegistryEntryIterateCreate(&cookie);
    if (err != noErr) return err;
    err = RegistryEntrySearch(&cookie, kRegIterSubTrees,
                               &entry, &done,
                               "device_type", "mac-io", 7);
    if (err != noErr || done) {
        RegistryEntryIterateDispose(&cookie);
        err = RegistryEntryIterateCreate(&cookie);
        if (err != noErr) return err;
        err = RegistryEntrySearch(&cookie, kRegIterSubTrees,
                                   &entry, &done,
                                   "name", "mac-io", 6);
    }
    RegistryEntryIterateDispose(&cookie);
    if (err != noErr || done) return -1;

    sz = sizeof(regProp);
    err = RegistryPropertyGet(&entry, "assigned-addresses",
                               regProp, &sz);
    if (err != noErr) {
        sz = sizeof(regProp);
        err = RegistryPropertyGet(&entry, "reg", regProp, &sz);
        if (err != noErr) return err;
    }
    gMacIOBase = regProp[2];
    return (gMacIOBase != 0) ? noErr : (OSStatus)-1;
}

/* ---- INIT Icon ---- */

static void ShowINITIcon(short iconID, Boolean advance)
{
    Handle      iconHandle;
    short       hPos;
    Rect        iconRect;
    GrafPort    myPort;
    GrafPtr     savePort;

    iconHandle = GetResource('ICN#', iconID);
    if (iconHandle == nil) return;
    hPos = *(short *)0x92C;
    if (hPos == 0) hPos = 8;
    GetPort(&savePort);
    OpenPort(&myPort);
    iconRect.left   = hPos;
    iconRect.right  = hPos + 32;
    iconRect.bottom = myPort.portBits.bounds.bottom - 8;
    iconRect.top    = iconRect.bottom - 32;
    PlotIconID(&iconRect, atNone, ttNone, iconID);
    if (advance) {
        hPos += 40;
        *(short *)0x92C = hPos;
    }
    ClosePort(&myPort);
    SetPort(savePort);
    ReleaseResource(iconHandle);
}

/* ---- Main ---- */

void main(void)
{
    OSStatus err;
    long gestaltResult;

    err = Gestalt(gestaltNameRegistryVersion, &gestaltResult);
    if (err != noErr) {
        ShowINITIcon(kIconFail_ID, true);
        return;
    }

    err = FindIOControllerBase();
    if (err != noErr || gMacIOBase == 0) {
        ShowINITIcon(kIconFail_ID, true);
        return;
    }

    /*
     * Enable all amplifier GPIO pins.
     *
     * These pins are in state 0x04 (data HIGH but output disabled)
     * after the screamer driver loads. The ROM had them enabled
     * for the startup chime, but the screamer driver turned off
     * the output enable bit.
     *
     * Writing 0x05 sets: bit 0 (output enable) + bit 2 (data HIGH).
     * The hardware responds with 0x07 (also sets bit 1 = active).
     */
    WriteGPIO(GPIO_AMP_1, GPIO_AMP_ON);   /* 0x6E */
    WriteGPIO(GPIO_AMP_2, GPIO_AMP_ON);   /* 0x6F */
    WriteGPIO(GPIO_AMP_3, GPIO_AMP_ON);   /* 0x70 */
    WriteGPIO(GPIO_AMP_4, GPIO_AMP_ON);   /* 0x79 */

    ShowINITIcon(kIconOK_ID, true);
}
