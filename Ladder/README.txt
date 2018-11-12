1. Move the 'Maps' folder to "C:/Program Files (x86)/StarCraft II/".
   For more maps use the !maps command on our discord or download them here https://github.com/Blizzard/s2client-proto#map-packs
   Remember to put the map files directly in /Maps/.
2. Add your bot to the Bots/LadderBots.json file. Follow the example configurations already there.
3. Execute LadderGUI.exe.
4. Select the matches you want to play.

For dentosalBot games python-sc2 must be installed first. I used anaconda for this. https://github.com/Dentosal/python-sc2
For pysc2Bot games pysc2 must be installed first. https://github.com/deepmind/pysc2
For ocraft bots you need to install Java 9 or newer.
You can play against your bot if you chose the "Human*" bost as FIRST! player.

Troubleshooting:
-After you press "Generate & Run" a console window opens but closes immediately. Nothing happens after that.
	Make sure the folder in Bots/ for your bot has exactly the same name you have given it in the LadderBots.json file.
-Your bot bot crashes immediately:
	Usually stderr is written to a log file inside the bot directory. Maybe you can find the error there. If not set the debug option in the LadderBots.json file to true. Now stdout is also written to file. Hopefully, you find the problem there.
-The two SC2 screens stay black:
	Go to Documents/StarCraft II/ and rename Variables.txt to Variables_old.txt. Start the normal SC2 client once. Try again. Be careful to not delete the Variables.txt in case you are playing the game. Those are the settings used ingame (scroll speed, language, etc).
-Anything else:
	Don't hesitate and ask on our Discord https://discord.gg/qTZ65sh
