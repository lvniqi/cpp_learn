#!/bin/bash

echo "function getRoxVimrcFile(){"
	vimrcEncodedContent="$(cat *vimrc | base64)"
	echo 'local roxVimrcFile=/tmp/rox_vimrc_$(echo "$(whoami)" | b64encode )'
	echo "echo '$vimrcEncodedContent' | b64decode > \$roxVimrcFile"
	echo "echo \"\$roxVimrcFile\""
echo "}"

echo "function getPluginsTgzEncodedContent(){"
	pluginsTgzEncodedContent="$(tar -cz plugins | base64)"
	echo "echo '$pluginsTgzEncodedContent'"
echo "}"

echo "function getPluginsTgzEncodedContentMd5sum(){"
	echo "echo '$(echo $pluginsTgzEncodedContent| md5sum)'"
echo "}"


cat init.sh

