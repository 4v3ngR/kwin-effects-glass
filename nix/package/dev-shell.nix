{
  mkShell,
  kwin-glass,
}:

mkShell {
  inputsFrom = [
    kwin-glass
  ];
}
