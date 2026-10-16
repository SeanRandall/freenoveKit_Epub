
This demo is a console application that can run upto 4 channels 
of text to speech. The text is read out from the file state01.inp.

To run the demo type : ttsdemo -c<NoOfChannels> at the prompt.
The demo needs the DLL "texttalk.dll" to execute.
For eg. ttsdemo -c1

The demo code can be modified to run in the synchrounous mode.
The default mode is async.

The DTMF keys used for the demo are :

DTMF 1 :   Increase Pitch by 20 Hz.
DTMF 2 :   Decrease Pitch by 20 Hz.
DTMF 3 :   Pause the speech output.
DTMF 4 :   Resume the speech output.
DTMF 5 :   Increase Speed by 15 wpm.
DTMF 6 :   Increase Speed by 15 wpm.
DTMF 7 :   Jump Forward by 60 char.
DTMF 8 :   Jump Backward by 60 char.
DTMF 9 :   Stop and re-start TextTalk.
DMTF * :   Terminates TextTalk demo on the channel.

To end the demo Press DMTF * on all channels.



