#!/bin/sh

echo "Checking for updates...";
tmpdir=$HOME/.cache/qalculate;
bindir=$PWD;
if [ ! -w "$bindir/qalc" ]; then
	bindir="$HOME/.local/bin";
	if [ ! -w "$bindir/qalc" ]; then
		bindir="$HOME/bin";
		if [ ! -w "$bindir/qalc" ]; then
			bindir=`which qalc`;
			if [ -w "$bindir" ]; then
				bindir=`dirname "$bindir"`;
			else
				bindir="";
			fi
		fi
	fi
fi
if [ -z "$bindir" ]; then
	echo "Writable Qalculate! binaries not found";
	exit 1;
fi
oldversion=`"$bindir/qalc" -v`;
mkdir -p $tmpdir;
if cd "$tmpdir"; then
	if curl -L -o "CURRENT_VERSIONS" https://qalculate.github.io/CURRENT_VERSIONS; then
		newversion=`grep libqalculate CURRENT_VERSIONS`;
		newversion=${newversion##*:}
		url=`grep linux-x86_64-selfcontained CURRENT_VERSIONS`;
		url=${url#*:}
		if [ -z $url ]; then
			url="https://github.com/Qalculate/qalculate-gtk/releases/download/v${newversion}/qalculate-${newversion}-x86_64.tar.xz";
		fi
		filename=${url##*/}
		rm CURRENT_VERSIONS;
	fi
fi
if [ -z $newversion ] || [ $newversion = $oldversion ]; then
	echo "No updates found";
	exit 0;
fi
echo "Updating Qalculate!...";
if curl -L -o ${filename} ${url}; then
	echo "Extracting files...";
	if tar -xJf ${filename}; then
		cd  qalculate-${newversion};
		if cp -f qalc "$bindir/"; then
			cp -f qalculate "$bindir/";
			cd ..;
			rm -r qalculate-${newversion};
			rm ${filename};
			exit 0;
		fi
		cd ..;
		rm -r qalculate-${newversion};
	fi
	rm ${filename};
fi
echo "Update failed";
exit 1
