#! /bin/bash
sizes=(1 4 512 1024 4096 8192);
for size in "${sizes[@]}"
do
  # first main
  lines=$(($RANDOM % 1000 + 100 | bc));
  ./main generate tmp1.txt $lines $size
  cp tmp1.txt tmp2.txt

  echo -e '\n\nlines ' $lines ' size ' $size >> results.txt
  echo -e '\nLIB' >> results.txt
  ./main sort lib tmp1.txt $lines $size >> results.txt
  echo -e '\nSYS' >> results.txt
  ./main sort sys tmp2.txt $lines $size >> results.txt

  echo -e '\nLIB' >> results.txt
  ./main copy lib tmp1.txt tmp11.txt $lines $size >> results.txt
  echo -e '\nSYS' >> results.txt
  ./main copy sys tmp2.txt tmp22.txt $lines $size >> results.txt



  lines=$(($RANDOM % 1000 + 100 | bc));
  ./main generate tmp1.txt $lines $size
  cp tmp1.txt tmp2.txt

  echo -e '\n\nlines ' $lines ' size ' $size >> results.txt
  echo -e '\nLIB' >> results.txt
  ./main sort lib tmp1.txt $lines $size >> results.txt
  echo -e '\nSYS' >> results.txt
  ./main sort sys tmp2.txt $lines $size >> results.txt

  echo -e '\nLIB' >> results.txt
  ./main copy lib tmp1.txt tmp11.txt $lines $size >> results.txt
  echo -e '\nSYS' >> results.txt
  ./main copy sys tmp2.txt tmp22.txt $lines $size >> results.txt

done
rm tmp*.txt
