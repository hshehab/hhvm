<?hh

function main() {
  echo "Entering main\n";
  foreach (array(1 => 1) as $k => $v) {
    break;
  }
  echo "Leaving main\n";
}

<<__EntryPoint>>
function main_501() {
main();
}
