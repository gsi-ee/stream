#  par1 - filename
#  par2 - trigkind

function proc {
   export USETEMP=0
   export CALTRIG=$2
   asffile=res/$1-$2.root
   export CALNAME=cal/$1-$2_
   export CALMODE=-1
   rm -f *.root $CALNAME????.cal 
   go4analysis -user "data/$1*.hld" -asf dummy.root -rate
   export CALMODE=0
   rm -f *.root $asffile
   go4analysis -user "data/$1*.hld" -asf $asffile -rate
}

#############################################################

#  par1 - filename

function global {
   export USETEMP=1
   export CALMODE=0
   export CALTRIG=0
   export CALNAME=cal/global_
   asffile=res/gl_$1.root
   rm -f $asffile
   go4analysis -user "data/$1*.hld" -asf $asffile -rate
}

##############################################################

# proc 40C 1
# proc 40C 13
# proc 42C 1
# proc 42C 13
# proc 44C 1
# proc 44C 13
# proc 46C 1
# proc 46C 13
# proc 48C 1
# proc 48C 13
# proc 50C 1
# proc 50C 13

global 40C
global 42C
global 44C
global 46C
global 48C
global 50C
