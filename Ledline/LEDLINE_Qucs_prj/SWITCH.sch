<Qucs Schematic 24.4.1>
<Properties>
  <View=0,0,1019,598,1,0,0>
  <Grid=10,10,1>
  <DataSet=SWITCH.dat>
  <DataDisplay=SWITCH.dpl>
  <OpenDisplay=0>
  <Script=SWITCH.m>
  <RunScript=0>
  <showFrame=0>
  <FrameText0=Название>
  <FrameText1=Чертил:>
  <FrameText2=Дата:>
  <FrameText3=Версия:>
</Properties>
<Symbol>
  <.PortSym 40 20 1 0 P1>
</Symbol>
<Components>
  <Vdc V1 1 690 280 18 -26 0 1 "5 V" 1>
  <R_SPICE R1 1 530 140 -26 15 0 0 "40 kOm" 1 "" 0 "" 0 "" 0 "" 0 "2" 1 "R" 1>
  <Switch S1 1 290 260 11 -26 0 1 "off" 0 "1 ms" 0 "1e-9" 0 "1e12" 0 "26.85" 0 "1e-6" 0 "spline" 0>
  <C_SPICE C1 1 390 260 17 -26 0 1 "" 1 "" 0 "" 0 "" 0 "" 0 "2" 1 "C" 1>
  <Port P1 1 150 140 -23 12 0 0 "1" 1 "analog" 0 "v" 0 "" 0>
</Components>
<Wires>
  <690 140 690 250 "" 0 0 0 "">
  <560 140 690 140 "" 0 0 0 "">
  <690 310 690 390 "" 0 0 0 "">
  <290 140 390 140 "" 0 0 0 "">
  <290 140 290 230 "" 0 0 0 "">
  <290 390 390 390 "" 0 0 0 "">
  <290 290 290 390 "" 0 0 0 "">
  <390 140 500 140 "" 0 0 0 "">
  <390 140 390 230 "" 0 0 0 "">
  <390 390 690 390 "" 0 0 0 "">
  <390 290 390 390 "" 0 0 0 "">
  <150 140 290 140 "" 0 0 0 "">
</Wires>
<Diagrams>
</Diagrams>
<Paintings>
</Paintings>
