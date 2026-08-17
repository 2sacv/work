# 手动升级


 1. tftp normal.tgz 到/tmp目录 (.66M/s)
 2. 解压出updateExt.sh，并升级

code:

    PKG=normal.nxp.tgz;
    cd /tmp && tftp -gr ${PKG} 192.168.2.41;
    sed -i '$s/[0-9a-z]\{32\}$//' ${PKG}; tar zxf upTools/updateExt.sh ${PKG}; upTools/updateExt.sh ${PKG}



# 2nd manually

    PKG=normal.nxp.tgz;
    cd /tmp && tftp -gr ${PKG} 192.168.2.41;
    sed -i '$s/[0-9a-z]\{32\}$//' ${PKG}; 
    size=`tail -c6 ${PKG}`
    sed -i '$s/[0-9a-z]\{6\}$//' ${PKG}; 
    tail -c${size} ${PKG}> updateExt.sh

    tar zxf upTools/updateExt.sh ${PKG}; upTools/updateExt.sh ${PKG}
