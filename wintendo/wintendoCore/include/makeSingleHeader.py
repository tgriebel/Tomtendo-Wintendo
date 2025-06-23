from os import listdir
from os.path import isfile, join

filenames = [
    'base.h', # Must be first
    'command.h',
    'image.h',
    'input.h',
    'log.h',
    'playback.h',
    'serializer.h',
    'time.h',
    'timer.h',
    'util.h',
    'interface.h' # Must be last
]

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

with open("tomtendoCore.h", "w") as f:
    f.write(combinedFile)