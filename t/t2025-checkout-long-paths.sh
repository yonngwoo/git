#!/bin/sh

test_description='checkout long paths on Windows

Ensures that Git for Windows can deal with long paths (>260) enabled via core.longpaths'

. ./test-lib.sh

if test_have_prereq NOT_MINGW
then
	skip_all='skipping MINGW specific long paths test'
	test_done
fi

test_expect_success setup '
	p=longpathxx && # -> 10
	p=$p$p$p$p$p && # -> 50
	p=$p$p$p$p$p && # -> 250

	path=${p}/longtestfile && # -> 263 (MAX_PATH = 260)

	blob=$(echo foobar | git hash-object -w --stdin) &&

	printf "100644 %s 0\t%s\n" "$blob" "$path" |
	git update-index --add --index-info &&
	git commit -m initial -q
'

test_expect_success 'checkout of long paths without core.longpaths fails' '
	git config core.longpaths false &&
	test_must_fail git checkout -f 2>error &&
	grep -q "Filename too long" error &&
	test_path_is_missing longpa~1/longtestfile
'

test_expect_success 'checkout of long paths with core.longpaths works' '
	git config core.longpaths true &&
	git checkout -f &&
	test_path_is_file longpa~1/longtestfile
'

test_expect_success 'update of long paths' '
	echo frotz >> longpa~1/longtestfile &&
	echo $path > expect &&
	git ls-files -m > actual &&
	test_cmp expect actual &&
	git add $path &&
	git commit -m second &&
	git grep "frotz" HEAD -- $path
'

test_expect_success cleanup '
	# bash cannot delete the trash dir if it contains a long path
	# lets help cleaning up (unless in debug mode)
	test ! -z "$debug" || rm -rf longpa~1
'

test_expect_success 'clean up path close to MAX_PATH' '
	SMITHS="$(cd .. && pwd -W)/smiths" &&
	if test -d "$SMITHS"
	then
		rm -rf "$SMITHS"
	fi &&

	SMITHS_LENGTH=$(echo "$SMITHS" | wc -c) &&
	if test 50 -ge $SMITHS_LENGTH
	then
		SUFFIX="$(echo smiths/workspace/ATM-hourly/SLAVE/build-win7-32 |
			tail -c $((51-$SMITHS_LENGTH)))" &&
		ROOT="$SMITHS/$SUFFIX" &&
		SUB1A=lodash.merge/node_modules &&
		SUB1=data/data/gui/html/common/jwe/node_modules/$SUB1A &&
		SUB2A=bind/node_modules/lodash._createwrapper/node_modules &&
		SUB2=lodash._basecreatecallback/node_modules/lodash.$SUB2A &&
		SUB3A=lodash._basecreate/node_modules/lodash._isnative &&
		SUB3=lodash._basecreatewrapper/node_modules/$SUB3A &&

		mkdir -p $ROOT &&
		(cd $ROOT &&
		 git init &&

		 mkdir -p $SUB1 $SUB2 $SUB3 &&
		 echo {} >> $SUB3/package.json &&
		 mv ${SUB3%%/*} $SUB2 &&
		 mv ${SUB2%%/*} $SUB1 &&

		 git config core.longpaths yes &&
		 git clean -fdx) &&
		rm -rf "$SMITHS"
	fi
'

test_done
