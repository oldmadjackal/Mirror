#!/bin/bash

###############################################################################################################

#Directory of certificates
CERT_FOLDER='/home/ruabop/Certs/'

#Password for certificate
Pass='1234567890'

################################################################################################################

#create $err and $err.tmp files
touch ${4}
touch ${4}.tmp

#checking arguments
if [ $# -gt 3 ]
then
        if [ -r $1 ]
        then
                cryptcp -decr -dn ${3} -nochain -pin ${Pass} ${1} ${2} > ${4}.tmp 2>&1
        else
                err_file_in=$(cat ${1} 2>&1|cut -d':' -f3)
                echo "ERROR:Cryptcp.exe call error:"${err_file_in} >${4}
		rm -f ${4}.tmp
                exit
        fi
else
        echo "ERROR: need more arguments">${4}
	rm -f ${4}.tmp
        exit
fi

#checking temporary file
if [ -r ${4}.tmp ]
then  
	echo "ok" > /dev/null 
else 
	err_read_tmp=$(cat ${4}'.tmp' 2>&1|cut -d':' -f3 )
	echo "ERROR:Decrypting result file read error">${4}
	rm -f ${4}.tmp
	exit
fi



#checking temporary file
ERROR0=$(grep -c '[ErrorCode: 0x00000000]' ${4}.tmp)
ERROR_CERT=$(grep -c 'Error: No certificate found' ${4}.tmp)
ERROR_ANY_CERT=$(grep -c 'Error: More than one certificate found' ${4}.tmp )


if [ $ERROR0 -gt 0 ]
then
        echo "SUCCESS" > /dev/null
else
        echo "ERROR:Success mark is absent in encrypting result file">${4}
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
exit
