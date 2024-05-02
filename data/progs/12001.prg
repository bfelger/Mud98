#PROG
vnum 12001
code if hastarget $i
mob echoat $q {_You have instructions to meet your old mentor in a clearing just to the east. There you{/will continue your training. Type '{*EXITS{_' to see what lies in that direction. Type '{*EAST{_'{/to go there.{x
else
    mob forget
endif
~
#END

