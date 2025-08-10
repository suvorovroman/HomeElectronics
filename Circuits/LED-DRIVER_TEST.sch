<Qucs Schematic 24.4.1>
<Properties>
  <View=106,-16,991,503,1.15141,0,0>
  <Grid=10,10,1>
  <DataSet=LED-DRIVER_TEST.dat>
  <DataDisplay=LED-DRIVER_TEST.dpl>
  <OpenDisplay=0>
  <Script=LED-DRIVER_TEST.m>
  <RunScript=0>
  <showFrame=0>
  <FrameText0=Название>
  <FrameText1=Чертил:>
  <FrameText2=Дата:>
  <FrameText3=Версия:>
</Properties>
<Symbol>
</Symbol>
<Components>
  <GND * 1 330 320 0 0 0 0>
  <Vdc V2 1 510 290 18 -26 0 1 "12 V" 1>
  <Vdc V1 1 160 290 18 -26 0 1 "5 V" 1>
  <R R1 1 510 190 15 -26 0 1 "31 Ohm" 1 "26.85" 0 "0.0" 0 "0.0" 0 "26.85" 0 "european" 0>
  <IProbe Pr2 1 230 110 -26 16 0 0>
  <Sub DRIVER 1 340 110 -26 21 0 0 "LED-DRIVER-MODEL.sch" 0>
  <.DC DC1 1 670 70 0 99 0 0 "26.85" 0 "0.001" 0 "1 pA" 0 "1 uV" 0 "no" 0 "150" 0 "no" 0 "none" 0 "CroutLU" 0>
  <IProbe Pr1 1 450 110 -26 16 1 2>
</Components>
<Wires>
  <160 320 330 320 "" 0 0 0 "">
  <330 320 510 320 "" 0 0 0 "">
  <510 220 510 260 "" 0 0 0 "">
  <510 110 510 160 "" 0 0 0 "">
  <480 110 510 110 "" 0 0 0 "">
  <160 110 160 260 "" 0 0 0 "">
  <160 110 200 110 "" 0 0 0 "">
  <370 110 420 110 "" 0 0 0 "">
  <260 110 310 110 "" 0 0 0 "">
</Wires>
<Diagrams>
  <Tab 650 420 300 200 3 #c0c0c0 1 00 1 0 1 1 1 0 1 1 1 0 1 1 315 0 225 1 0 0 "" "" "">
	<"ngspice/i(pr1)" #0000ff 0 3 1 0 0>
	<"ngspice/i(pr2)" #0000ff 0 3 1 0 0>
  </Tab>
</Diagrams>
<Paintings>
</Paintings>
