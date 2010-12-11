require "mkmf"

def crash(str)
  puts "extconf failure: "+str
  exit 1
end

unless have_library('mp3lame','lame_init')
  crash("Requencer needs libmp3lame")
end

unless have_header('lame/lame.h')
  crash("Requencer needs libmp3lame development headers")
end

$CFLAGS << "-msse3 -std=c99 -pedantic"

create_makefile("requencer/requencer")
