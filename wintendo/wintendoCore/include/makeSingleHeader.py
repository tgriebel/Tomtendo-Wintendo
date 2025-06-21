from os import listdir
from os.path import isfile, join

filenames = ['command.h', 'image.h', 'input.h', 'log.h', 'playback.h', 'serializer.h', 'time.h', 'timer.h', 'util.h', 'interface.h']

combinedFile = ""

for fname in filenames:
    f = open( join( "tomtendo", fname) )
    fileString = f.read()
    
    for includeName in filenames:
        fileString = fileString.replace('#include "' + includeName + '"', '//#include "' + includeName + '"')     
    
    combinedFile += "\n"
    combinedFile += "/*=======================================================\n"
    combinedFile += "* BEGIN - " + fname + "\n"
    combinedFile += "*=======================================================*/"
    combinedFile += "\n"
    combinedFile += fileString
    
    combinedFile += "\n"
    combinedFile += "/*=======================================================\n"
    combinedFile += "* END - " + fname + "\n"
    combinedFile += "*=======================================================*/"
    combinedFile += "\n"

with open("tomtendo.h", "w") as f:
    f.write(combinedFile)