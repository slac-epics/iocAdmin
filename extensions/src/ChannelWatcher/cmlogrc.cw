# cmlog config file for Channel Watcher
# start cmlog with: cmlog -u -f cmlogrc.cw &
#
# Type          Title          Tag          Width
Col     Time       cmlogTime   170
Col     Sys        facility    120
Col     Error      code        40
Col     Message    text        500
Col     IOC        host        100
Col     Channel    device      200
Col     Alias      message     200
Col     Sevr       severity    40
Col     Stat       status      40
Col     Value      value       170
#
# Window Width
windowWidth 1700
#
# Window Height
windowHeight  400
#
# Buffer size for updating the message
ubufsize     2000
#
# Default query message
#   Example: queryMessage "none" -36:0 now
queryMessage "none"
#
# Default update selection message
#   Examples: updateMessage    "Status == 2"
#     updateMessage "Name like 'ODF' && Domain like 'ORC'"
updateMessage    "none"
#
# code conversion information
#codeConversion "none"  Status
#{
#0          Succ          Green
#1          Info          Blue
#2          Warning       LightBlue
#3          Error         Yellow      flash
#4          Fatal         Red         flash
#}
#
