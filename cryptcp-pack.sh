#!/bin/bash


###############################################################################################

#directory of certificates
CERT_FOLDER='/opt/ethereum_mirror/certs/'

###############################################################################################



#create result and temporary result files 
touch ${4}
touch ${4}.tmp
ch=0

#making list of certificates
LIST_CERT=$(echo ${3} |tr "," " ")

CRYPTCP='cryptcp -encr'
LIST_CERT_COMMAND=''

for i in $LIST_CERT
do
        LIST_CERT_COMMAND=$LIST_CERT_COMMAND" -f "$CERT_FOLDER$i".cer"
	let ch++
done

if [ $# -gt 3 ]
then
        if [ -r $1 ]
        then
                $CRYPTCP $LIST_CERT_COMMAND  -nochain ${1} ${2} > ${4}.tmp 2>&1
        else
		err=$(cat ${1} 2>&1|cut -d':' -f3)
		echo 'ERROR:Cryptcp.exe call error:'$err >${4}
		exit
	fi
else
        echo "need more arguments">${4}
        exit
fi


#checking temporary file
ERROR0=$(grep -c '[ErrorCode: 0x00000000]' ${4}.tmp)
ERROR1=$(grep -c $ch ${4}.tmp)
ERROR_CERT=$(grep -c 'Error: No certificate found' ${4}.tmp)
ERROR_ANY_CERT=$(grep -c 'Error: More than one certificate found' ${4}.tmp )



if [ $ERROR0 -gt 0 ]
then
	echo "another error">/dev/null
else
        echo "ERROR:Success mark is absent in encrypting result file">${4}
fi


if [ $ERROR1 -gt 0 ]
then
	echo "another error">/dev/null
else
        echo "ERROR:insufficient number of certificates used, details in file:">${4}
fi


if [ $ERROR_CERT -gt 0 ]
then
        echo "Error: No certificate found" > ${4}
        rm -f ${4}.tmp
        exit
fi

if [ $ERROR_ANY_CERT -gt 0 ]
then
        echo "Error: More than one certificate found" > ${4}
        rm -f ${4}.tmp
        exit
fi

#removing temporary file
rm -f ${4}.tmp

#writing success record
STR=$(cat ${4}|wc -l)
if [ $STR -eq 0 ]
then
	echo "SUCCESS">${4}
fi
