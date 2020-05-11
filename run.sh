echo "executing waf, stdout ==> build/output.txt, stderr ==> build/errput.txt"
../../waf --out=scratch/myproject/ --run  myproject > build/output.txt 2>build/errput.txt