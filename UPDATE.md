# Update

## From sources
```
VERSION=5.7.4
ARCHIVE_NAME=homebank-${VERSION}.tar.gz
curl -o ${ARCHIVE_NAME} https://www.gethomebank.org/public/sources/${ARCHIVE_NAME}
tar -xvf ${ARCHIVE_NAME}
rm ${ARCHIVE_NAME}
HOMEBANK_VERSION_FOLDER=homebank-${VERSION}
rsync -a ${HOMEBANK_VERSION_FOLDER}/* .
rm -r ${HOMEBANK_VERSION_FOLDER}
```
