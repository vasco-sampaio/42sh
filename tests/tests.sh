#!/bin/sh

function test
{
    echo "$1"
    expected=$(cat $2 | dash)
    retexp=$?
    tmp2=$(cat $2 | ../builddir/42sh)
    ret=$?
    if [ "$expected" = "$tmp2" ] && [ $retexp -eq $ret ]; then
        echo "$(tput setaf 2)OK"
        tput sgr0
    else
        echo "$(tput setaf 1)FAILED"
        if [ $retexp -eq $ret ]; then
            echo "expected : "
            echo "$(tput setaf 3)$expected"
            echo "$(tput setaf 1)but got :"
            echo "$(tput setaf 3)$tmp2"
            tput sgr0
        else
            echo "expected : "
            echo "$(tput setaf 3)$retexp"
            echo "$(tput setaf 1)but got :"
            echo "$(tput setaf 3)$ret"
            tput sgr0
        fi

        tput sgr0
    fi
}

echo "" && test "--IF THEN CMD--" "test_files/test1"
echo "" && test "--IF THEN ELSE--" "test_files/test2"
echo "" && test "--IF THEN ELIF THEN ELSE--" "test_files/test3"
echo "" && test "--REDIR DEV NULL--" "test_files/test4"
echo "" && test "--REDIR DEV NULL APPENDED--" "test_files/test5"
echo "" && test "--REDIR 2 INTO DEV NULL--" "test_files/test6"
echo "" && test "--REDIR IF ECHO IN &2--" "test_files/test7"
echo "" && test "--SIMPLE ECHO TR PIPE--" "test_files/test8"
echo "" && test "--SIMPLE &&--" "test_files/test9"
echo "" && test "--SIMPLE ||--" "test_files/test10"
echo "" && test "--IF TRUE || FALSE--" "test_files/test11"
echo "" && test "--IF FALSE || TRUE--" "test_files/test12"
echo "" && test "--IF TRUE && FALSE--" "test_files/test13"
echo "" && test "--IF FALSE && TRUE--" "test_files/test14"
echo "" && test "--IF TRUE && TRUE && FALSE--" "test_files/test15"
echo "" && test "--IF FALSE || FALSE || FALSE--" "test_files/test16"
echo "" && test "--IF FALSE || FALSE || TRUE--" "test_files/test17"
echo "" && test "--IF FALSE ||| TRUE --" "test_files/test18"
echo "" && test "--IF LS && ECHO TRUE --" "test_files/test19"
echo "" && test "--IF FALSE || ECHO YES THEN LS || FALSE && ECHO OUI --" "test_files/test20"
