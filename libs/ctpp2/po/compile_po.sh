#!/bin/sh -x

LOCALES="CP866 KOI8-R CP1251 UTF-8"

for LOCALE in ${LOCALES}
do
	TMPFILE=`mktemp po/ctpp2.tmp.XXXX`
	echo ${TMPFILE}
	mkdir -p "po/ru_RU.${LOCALE}/LC_MESSAGES"
	iconv -f UTF-8 -t ${LOCALE} < po/ctpp2.ru_RU.UTF-8.po > ${TMPFILE}
	sed -e "s/charset=UTF-8/charset=${LOCALE}/g" < ${TMPFILE} > "po/ctpp2.ru_RU.${LOCALE}.po"
	rm -f ${TMPFILE}

	msgfmt -c -v -o "po/ru_RU.${LOCALE}/LC_MESSAGES/ctpp2.mo" "po/ctpp2.ru_RU.${LOCALE}.po"
done
