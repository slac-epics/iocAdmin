/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution. 
\*************************************************************************/

static char *helpIndex[] = {
    "Change channel value",
    "Change channel gain",
    "Connect a channel",
    "Copy and Paste",
    "Disconnect a channel",
    "Load configuration",
    "Memory",
    "Reset Max & Min",
    "Save configuration",
    "Select a channel",
    "Select Multiple Channels",
    "Using a button Panel",
    "Using a knob",
    "Using the mouse",
    "Using a slider",
    "Using a text Entry",
    "Using Up & Down Arrows",
    };

#define CHANGE_CHANNEL_VALUE \
"CHANGE A CHANNEL'S VALUE\n\
-----------------------------\n\n\
  Select the channel which you want to change value. There are four\
 ways you can do it.\n\n\
  1) Move the pointer over the value field of the selected channel.\
  Click the RIGHT mouse button once to bring up a popup menu with four\
 buttons which are 'increment', 'decrement', 'set' and 'gain'.  Click\
 the LEFT mouse button with the mouse pointer over the 'increment'\
 button once to increase the value by one increment specified in the\
 'gain' dialog.  Same as above, click the 'decrement' button once to\
 decrease the value by one increment, click the 'set' button once to\
 bring up the adjust dialog and click the 'gain' button once to bring\
 up the gain dialog.\n\n\
  2) Move the pointer over the value field of the selected channel\
 and do a double click on the LEFT mouse button to bring up the adjust\
 dialog.\n\n\
  3) Use the 'up' and 'down' arrow buttons at the top of the window of\
 the selected channel to increase or decrease the value by one increment\
 specified in the 'gain' dialog.  If the buttons are disable, the arrow\
 is dimmed.  If the buttons are enable, the arrow is solid black.  To\
 enable the 'up' and 'down' arrow buttons, move the mouse pointer over\
 either buttons, click the RIGHT mouse button to bring up the popup menu\
 with the 'enable' and 'disable' buttons.  Click the LEFT mouse button\
 with the mouse pointer over the 'up' arrow button to increase the value\
 and over the 'down' arrow button to decrease the value by one increment\
 specified in the 'gain' dialog.\n\n\
  4) If your machine has the SUN DIALS installed, you can use the SUN\
 DIALS to change the value.  The number at the top left corner of the\
 channel is the corresponding dial number in the SUN DIALS.  There is\
 a 'knob' icon at the top right corner to indicate whether the knob\
 is enable or disable.  When the icon is disable, the icon is dimmed.\
 Otherwise, the icon is in black color.  To enable or disable the SUN\
 DIALS, move the mouse pointer over the 'knob' icon and click the RIGHT\
 mouse button to bring up a popup menu with the 'enable' and 'disable'\
 buttons.  Click the LEFT mouse button with the mouse pointer over the\
 'enable' button and 'disable' button to enable or disable the SUN DIAL\
 for the selected channel."

#define CHANGE_CHANNEL_GAIN \
"CHANGE THE GAIN OF THE CHANNEL\n\
------------------------------\n\n\
  Select the channel(s) which you want to modify the gain. The gain function\
 only works if the channel is connected and the channel itself is an analog\
 channel. Pull down the 'Misc' menu then select the 'Gain Settings' or move the\
 mouse pointer over the value field of the selected channel, bring up the\
 popup menu by clicking the right mouse button once, then press the gain button\
 A gain control dialog will pop up. The dialog consists of one display area,\
 three text entry fields, and three buttons. The display area itself consists\
 of a number and six square buttons. The number is the current value of the\
 channel. The short bar just below the number is an indicator of which\
 digit the change of a minimum step will affect. The six square buttons\
 provide the basic function to control the gain. The left and right arrow\
 moves the gain one digit left and one digit right respectively.  The other\
 four buttons multiply or divide the current gain by hundred or ten times.\
 Besides using the square button, you can type in the step size in the\
 text entry box with the label 'incremental step'.  Click the 'apply' button\
 to use your current step size. The 'default' button will use the setting\
 in the IOC. The default step size is the value of the last digit of the number."

#define CONNECT_A_CHANNEL \
"CONNECT A CHANNEL TO A PROCESS VARIABLE\n\
---------------------------------------\n\n\
  Select the channel(s) you want to use. There are three ways to bring up\
 the channel name dialog.\n\n\
1) Move the mouse pointer over the CHANNEL NAME FIELD of the channel you\
 want to use, bring up the channel dialog by double click the LEFT button.\n\n\
2) Move the mouse pointer over the CHANNEL NAME FIELD of the channel you\
 want to use, bring up the popup menu by clicking the RIGHT mouse button\
 once. Select the 'connect' to bring up the channel name dialog.\n\n\
3) Select the 'Edit PV Name' of the EDIT Menu in the menu bar to bring\
 up the channel name dialog.  This method is useful for bringing up\
 several channel name dialogs for multiple channels.\n\n\
 After bringing up the channel name dialog, enter the process variable\
 name into the text box; and hit 'return' or click 'OK' to connect the\
 channel. If the field name of the process variable name is not specified,\
 the 'VAL' field will be used as default. The MONITOR ICON will change\
 color from grey to black if the connection is successful. Otherwise, the\
 KM will stop trying to connect after 60 seconds time out."

#define COPY_AND_PASTE \
"COPY AND PASTE A PROCESS VARIABLE NAME\n\
--------------------------------------\n\n\
  If you want to copy and paste a process variable from one channel to\
 another channel, just move the mouse pointer over the channel containing\
 the process variable you want to copy; hold down the middle button and\
 then drag the mouse pointer over the channel which you want to paste the\
 process variable; release the mouse button. A channel name dialog box will\
 come up.  Click 'OK' to accept the process variable.  This copy function\
 also works between the KM and all control elements of MEDM."

 
#define DISCONNECT_A_CHANNEL \
"DISCONNECT A CHANNEL TO A PROCESS VARIABLE\n\
------------------------------------------\n\n\
There are two ways to disconnect a channel :\n\n\
1) Move the mouse pointer over the CHANNEL NAME FIELD of the channel\
 you want to disconnect.  click the RIGHT mouse button once to bring\
 up the popup menu and select 'disconnect'.\n\n\
2) Select channel(s) you want to disconnect.  Pull down the the 'Edit'\
 menu from the menu bar and select the 'Disconnect'."

#define LOAD_CONFIGURATION\
"LOAD CONFIGURATION\n\
------------------\n\n\
  You can load the configuration from a file.  Select 'Open' in the 'file'\
 menu. Km replaces the process variable name, gain, memory, etc., of each\
 channel from a file you specified." 

#define MEMORY \
"MEMORY FUNCTION\n\
---------------\n\n\
  You can store the current value into the memory or recall the value\
 from the memory you stored previously.  Each channel has its own memory.\
 There are two ways to invoke the function :\n\n\
1) Move the mouse pointer over the MEMORY icon, click RIGHT mouse button\
 once to bring up the popup menu.  Select the 'store' to store the current\
 value into the memory, select 'recall' to restore the channel value with\
 the memory value or 'clear' to clear the memory.\n\n\
2) Select the channel(s) you want to perform the fucntion. Select the\
 'Misc' menu in the menu bar. Pull down the 'memory' sub-menu from the\
 'Misc' menu and select the 'store', 'recall' or 'clear' function.\n\n\
  When the MEMORY icon is solid black, it means that a value has been\
 stored in the memeory."

#define RESET_MAX_AND_MIN \
"RESET MAX AND MIN\n\
-----------------\n\n\
  Maximum and minimum value can be reset by the folloing methods :\n\
1) Move the mouse pointer over the BAR GRAPH area, click RIGHT mouse button\
 once to bring up the popup menu.  Select the 'reset' to reset the \
 maximum and minimum value to the current value.\n\n\
2) Select the channel(s) you want to perform the fucntion. Pull down the\
 'Misc' menu in the menu bar and select the 'Reset max & min'."

#define SAVE_CONFIGURATION\
"SAVE CONFIGURATION\n\
------------------\n\n\
  You can save the current configuration of each channel into a file.  Select\
 'Save' or 'Save As' in the 'file' menu. The only different between 'Save'\
 and 'Save As' are where 'Save' use the current filename and 'Save As' ask for\
 a new filename. Km saves the process variable name, gain, memory, etc. of the\
 channel into a file you specified.  It is recommended to use the file name\
 with the '.cnfg' suffice." 

#define SELECT_A_CHANNEL \
"SELECT A CHANNEL\n\
----------------\n\n\
  Move the pointer over the rectangle display area of the channel which you\
 want to select, and do a single click on the 'left' mouse button. The selected\
 channel(s) is indicated by highlighting edge of the channel\
 and reversing color of the channel number from blue to yellow."

#define SELECT_MULTIPLE_CHANNELS \
"SELECT MULTIPLE CHANNELS\n\
------------------------\n\n\
  You can do a multiple selection by holding down the 'Control' key and doing\
 a single click on the 'left' mouse button when mouse pointer is over\
 the channel you want to select."

#define USING_BUTTON_PANEL \
"USING BUTTON PANEL TO CHANGE STATE VALUE\n\
----------------------------------------\n\n\
  Select the state you want to go by hitting the diamond button beside that\
 state label."

#define USING_A_KNOT \
"USING A KNOT TO CHANGE A NUMERICAL VALUE\n\
----------------------------------------\n\n\
  The knob only works for analog channel.  By default, the knob will be\
 enable after the an analog channel is connected.  For information on\
 adjusting the gain, please read the 'Change channel gain'.\n\n\
There are two ways to enable or disable the knob :\n\n\
1) Move the mouse pointer over the KNOB icon, click RIGHT mouse button\
 once to bring up the popup menu.  Select the 'enable' or 'disable' to\
 activate or de-activate the knob to the corresponding channel.\n\n\
2) Select the channel(s) you want to perform the fucntion. Select the\
 'Misc' menu in the menu bar. Pull down the 'knob' sub-menu from the\
 'Misc' menu and select the 'enable' or 'disable' to activate or\
 de-activate the knob to the corresponding channel(s).\n\n\
Turning the knob clockwise will increase the gain and turning the\
 kob anti-clockwise will decrease the gain."

#define USING_A_TEXT_ENTRY \
"USING AN TEXT ENTRY TO CHANGE A VALUE\n\
-------------------------------------\n\n\
  Enter the value into the text entry.  For the numerical channel, only\
 number will be accepted." 

#define USING_THE_MOUSE\
"USING MOUSE IN KM\n\
-----------------\n\n\
  Km uses the mouse very consistently.  The followings are some description\
of mouse operations the user will perform when using the km within the\
display area.\n\n\
SELECT A CHANNEL\n\
Move the mouse until the mouse pointer over a channel, a rectangle with a\
 circled number at the top left corner and the words 'Channel Name', 'Unknown',\
 etc.  The mouse pointer can be anywhere inside the rectangle. Then click\
 the LEFT mouse button once.  The action click means press down the mouse\
 button and then release the button.\n\n\
PRESS A BUTTON\n\
Move the mouse pointer over a button and click the LEFT mouse button once. A\
 button is rectangle containing a label or an image with 3-D effect on its\
 edge.\n\n\
BRING UP A POPUP MENU\n\
Move the mouse pointer over an object and click the RIGHT mouse button once.\
 The object can be an icon or a label within the display area." 

#define USING_A_SLIDER \
"USING A SLIDER TO CHANGE A NUMERICAL VALUE\n\
-------------------------------------------\n\n\
  Grab (move the mouse pointer over the slider and hold down the LEFT\
 mouse button) and move the rectangle in the rail to change\
 the value of the channel.  Moving the rectangle left will decrease the\
 channel value and moving right will increase the value.  Clicking the LEFT\
 or RIGHT arrows in the ends of the slider decrease or increase the channel\
 value by one increnment specified in the gain dialog.  The 'center' button\
 will position the 'slider' in the center of the rail if the upper and\
 lower operating limits are greater than the operating range of the\
 slider"

#define USING_UP_DOWN_ARROWS \
"USING A KNOT TO CHANGE A NUMERICAL VALUE\n\
----------------------------------------\n\n\
  The up and down arrows only works for analog channel.  By default,\
 the arrows will be enable after the an analog channel is connected.\
 For information on adjusting the gain, please read the 'Change channel\
 gain'.\n\n\
There are two ways to enable or disable the knob :\n\n\
1) Move the mouse pointer over the UP ARROW or DOWN ARROW icon,\
 click RIGHT mouse button once to bring up the popup menu.  Select\
 the 'enable' or 'disable' to activate or de-activate the arrows\
 to the corresponding channel.\n\n\
2) Select the channel(s) you want to perform the fucntion. Select the\
 'Misc' menu in the menu bar. Pull down the 'Up & Down Arrow' sub-menu\
 from the 'Misc' menu and select the 'enable' or 'disable' to activate or\
 de-activate the knob to the corresponding channel(s).\n\n\
  Click the UP ARROW icon with the LEFT mouse button to increase the channel\
 value by one increment specified in the gain dialog, or the DOWN ARROW icon\
 to decrease the channel value by one increment. Hold down the UP ARROW or\
 the DOWN ARROW icon with the LEFT mouse button will increase or decrease\
 the channel value continuously until the releasing the mouse button."


char *helpContext[] = {
    CHANGE_CHANNEL_VALUE,
    CHANGE_CHANNEL_GAIN,
    CONNECT_A_CHANNEL,
    COPY_AND_PASTE,
    DISCONNECT_A_CHANNEL,
    LOAD_CONFIGURATION,
    MEMORY,
    RESET_MAX_AND_MIN,
    SAVE_CONFIGURATION,
    SELECT_A_CHANNEL,
    SELECT_MULTIPLE_CHANNELS,
    USING_BUTTON_PANEL,
    USING_A_KNOT,
    USING_THE_MOUSE,
    USING_A_SLIDER,
    USING_A_TEXT_ENTRY,
    USING_UP_DOWN_ARROWS,
};



#define WHAT_IS_KM_BACKGROUND\
"BACKGROUND"

#define WHAT_IS_KM_NAME_FIELD\
"VARIABLE NAME"

#define WHAT_IS_KM_VALUE_FIELD\
"VARIABLE VALUE"

#define WHAT_IS_KM_STATUS_FIELD\
"VARIABLE ALARM STATUS"

#define WHAT_IS_KM_SEVERITY_FIELD\
"ALARM SEVERITY"

#define WHAT_IS_KM_DIGIT_FIELD\
"KNOB NUMBER"

#define WHAT_IS_KM_ARROW_UP_FIELD\
"UP ARROW ICON"

#define WHAT_IS_KM_ARROW_DOWN_FIELD\
"DOWN ARROW ICON"

#define WHAT_IS_KM_MEMORY_FIELD\
"MEMORY ICON"

#define WHAT_IS_KM_KNOB_FIELD\
"KNOB ICON"

#define WHAT_IS_KM_MONITOR_FIELD\
"MONITOR ICON"

#define WHAT_IS_KM_HIST_FIELD\
"BAR GRAPH BACKGROUND"

#define WHAT_IS_KM_HIST_HOPR_FIELD\
"BAR GRAPH UPPER LIMIT"

#define WHAT_IS_KM_HIST_LOPR_FIELD\
"BAR GRAPH LOWER LIMIT"

#define WHAT_IS_KM_HIST_MIN_FIELD\
"MINIMUM VALUE RECORDED"

#define WHAT_IS_KM_HIST_MAX_FIELD\
"MAXIMUM VALUE RECORDED"

#define WHAT_IS_KM_HIST_MEM_FIELD\
"VALUE IN MEMORY"

#define WHAT_IS_KM_HIST_MIN_ICON\
"MINIMUM INDICATOR"

#define WHAT_IS_KM_HIST_MAX_ICON\
"MAXIMUM INDICATOR"

#define WHAT_IS_KM_HIST_MEM_ICON\
"MEMORY VALUE INDICATOR"

#define WHAT_IS_KM_HIST_BARGRAPH\
"BAR GRAPH"


char *whatIs[] = {
    WHAT_IS_KM_BACKGROUND,
    WHAT_IS_KM_NAME_FIELD,
    WHAT_IS_KM_VALUE_FIELD,
    WHAT_IS_KM_STATUS_FIELD,
    WHAT_IS_KM_SEVERITY_FIELD,
    WHAT_IS_KM_DIGIT_FIELD,
    WHAT_IS_KM_ARROW_UP_FIELD,
    WHAT_IS_KM_ARROW_DOWN_FIELD,
    WHAT_IS_KM_MEMORY_FIELD,
    WHAT_IS_KM_KNOB_FIELD,
    WHAT_IS_KM_MONITOR_FIELD,
    WHAT_IS_KM_HIST_FIELD,
    WHAT_IS_KM_HIST_HOPR_FIELD,
    WHAT_IS_KM_HIST_LOPR_FIELD,
    WHAT_IS_KM_HIST_MIN_FIELD,
    WHAT_IS_KM_HIST_MAX_FIELD,
    WHAT_IS_KM_HIST_MEM_FIELD,
    WHAT_IS_KM_HIST_MIN_ICON,
    WHAT_IS_KM_HIST_MAX_ICON,
    WHAT_IS_KM_HIST_MEM_ICON,
    WHAT_IS_KM_HIST_BARGRAPH,
};

#define GETTING_START \
"GETTING START\n\
-------------\n\n\
  KM, Knob Manager, is an utility running on UNIX under X window.  By using\
 the km, one can read and modify the value of any process variable of any\
 database on any Input Output Controller running in the same subnet. If the\
 SUN DIALS, a knob box with eight knobs which hooks up to the workstation\
 serial port, is installed, the knobs can be used to adjust the process\
 variable values.\n\n\
  The KM is divided into three area: the MENU BAR, the MESSAGE BOX and the\
 DISPLAY.  The top rectangle with the words 'file', 'edit','format','misc'\
 and 'help' is the MENU BAR, the second rectangle is the MESSAGE BOX and\
 the bottom big rectangle is the DISPLAY which is sub-divided into eight\
 rectangles called CHANNEL.  Each CHANNEL has a circled number at the top\
 left corner, several labels and a smaller rectangle within it.  Each\
 circled number at the top left corner of each channel indicates the\
 corresponding physical knob in the knob box used by this channel.\n\n\
  Under the circled number, there are four fields labeled as 'Channel\
 Name', 'Unknown', 'No Alarm' and 'No Severity'.  The first field called\
 NAME FIELD shows the EPICS's process variable name; the second field\
 called VALUE FIELD shows the process variable's value; the third field\
 called the STATUS FIELD shows the process variable's alarm status and the\
 last field called the SEVERITY FIELD shows the process variable's alarm\
 severity.\n\n\
  At the top right corner, there are five status icons. Starting from the\
 right, they are MONITOR icon, KNOB icon, MEMORY icon, DOWN ARROW icon and\
 UP ARROW icon.  The color of the icons represents the state of status.\
 The solid black means ENABLE or ACTIVE, and the light grey means DISABLE\
 or INACTIVE.  The MONITOR icon is an indicator of the channel being\
 monitored.  The KNOB icon indicates the knob for this channel being active\
 or inactive.  The MEMORY icon indicates whether any value is stored in\
 the memory of this channel.  The UP and DOWN ARROW icons indicates\
 whether the icons themselves are active.\n\n\
  To get start, please look up the following topics under the 'index item'\
 submenu under the help menu:\n\
    - using the mouse.\n\
    - connect a channel.\n\
    - change channel value."


