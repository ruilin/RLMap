sed 's/ \[/|/;s/\]  "/|/;s/" /|/;s/\.mwm/|/' | awk '!x[$0]++'