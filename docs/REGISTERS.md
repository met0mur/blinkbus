Legend:
```
XX 		register name 		ACCESS			- purpose
		:XX-XX bits range 	:XX-XX coil range		- purpose
		:XX bits num 		:XXcoil num		- purpose
```
List:
```
00 		controll			RW	- setting controll command (like @load_defaults@)
	
01 		processor IO		RW	- zone processor remote controll
		:00-07		:16-23		- each processor SWITCH state
		:08-15		:24-31		- each processor SENSOR state
	
02 		master settings		W	- general settings flags
		:00			:32			- master switch. disable all zones when swithig off
		:01			:33			- day mode. zones ignore all sensors
		:02			:34			- evening mode. zones activated by sensors swithing to 100% pwm
		...
		:05			:37			- gesture lag. waiting for more input before activating gesture
	
03 		analog IO			R	- pin state monitor
		:00-07		:48-55		- each analog input channel state
		:08-15		:56-63		- each analog output channel state

10-17	analog to processor W 	- map analog input signals to processors. each bit represents target processor
	    :00-07					- map signal as SWITCH
		:08-15					- map signal as SENSOR

18 		lowPassMs			W	- sets minimal time interval between analog signal changeing

20-27	processor to analog W 	- map processors output signals to analog output
	    :00-07					- each bit represents target analog channel
		:08-10					- MergeRule

28 		intervalSmallMs		W	- sets gesture small interval
29 		intervalBigMs		W	- sets gesture big interval

30-37	analogToGestureMap	W 	- map analog input signals to gesture
	    :00-07					- each bit represents target gesture

40-47	gestureToSceneMap	W 	- represents gesture settings & map to scenes
	    :00-02					- action: Off = 0, On = 1, Half = 2, Min = 3, Toggle = 4
		:03						- select output of scene applying. zone = 1, analog = 0
		:04						- rotate selectes scenes for each gesture activation
		:05-07					- gesture type: OneClick = 0, DoubleClick, TripleClick, MediumClick, Hold, DoubleHold
		:08-15					- map to scenes

48		sceneActivation		RW	- last selected gesture copy

49		sceneActivFlags		RW	- 
		:08			:0792		- coil tells whether we have activated the selected gesture. needs for remote scene activation

50-57	scenes				W 	- represents template to applying a gesture
	    :00-07					- each channel activation state (1 - apply action, 0 - disable)
		:08-15					- each channel activation mask (1 - touch, 0 - do not)

60-67	pwmState			RW 	- 0-255 value of current pwm for each analog channel

90		PwmMinLevel			R 	- configuring value setting when processor in MIN state

91		PwmHalfLevel		R 	- configuring value setting when processor in HALF state

```