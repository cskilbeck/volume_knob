{ AddFootprints.pas - Altium DelphiScript
  Adds SOD-323, SOD-123FL, SOT-363 and SOT-89-3 footprints to the CURRENTLY OPEN PcbLib.
  Geometry taken from the EasyEDA/LCSC footprints for:
    SOD-323   C2128      1N4148WS   pads K / A   (cathode = left, matches symbol pins K/A)
    SOD-123FL C19077512  SMF18A     pads K / A   (cathode = left; EasyEDA numbers it 2=K 1=A)
    SOD-523   C85382     PESD24VS1UB pads K / A  (cathode = left)
    SOT-363   C5190214   2N7002DW   1=S1 2=G1 3=D2 4=S2 5=G2 6=D1 (pin 1 bottom-left)
    SOT-89-3  C20628877  78L05      1=OUT 2=GND(tab) 3=IN
  Usage: open resistive.PcbLib, then File > Run Script... > browse to this file > AddFootprints.
  Existing footprints with the same name are skipped.
  Silkscreen on Top Overlay, courtyard on Mechanical 15, 3D STEP bodies on Mechanical 13. }

Const
  MODEL_DIR = 'E:\dev\hid_kbd\pcb\resistive\3D\';
  SILK_W    = 0.15;
  CRT_W     = 0.05;

Var
  Lib   : IPCB_Library;
  Board : IPCB_Board;
  Comp  : IPCB_LibComponent;

{ PcbLib footprint coordinates are relative to the footprint origin - no board origin offset }
Function PX(V : Double) : Integer;
Begin
  Result := MMsToCoord(V);
End;

Function PY(V : Double) : Integer;
Begin
  Result := MMsToCoord(V);
End;

Procedure AddObj(Obj : IPCB_Primitive);
Begin
  Comp.AddPCBObject(Obj);
  PCBServer.SendMessageToRobots(Comp.I_ObjectAddress, c_Broadcast, PCBM_BoardRegisteration, Obj.I_ObjectAddress);
End;

Function StartComp(Name : String; Descr : String; HeightMM : Double) : Boolean;
Begin
  Result := False;
  If Lib.GetComponentByName(Name) <> Nil Then
  Begin
    ShowMessage('Footprint ' + Name + ' already exists - skipped');
    Exit;
  End;
  Comp := PCBServer.CreatePCBLibComp;
  Comp.Name := Name;
  Comp.Description := Descr;
  Comp.Height := MMsToCoord(HeightMM);
  Lib.RegisterComponent(Comp);
  Result := True;
End;

Procedure AddPad(Name : String; X, Y, W, H : Double; Shape : Integer);
Var
  P : IPCB_Pad;
Begin
  P := PCBServer.PCBObjectFactory(ePadObject, eNoDimension, eCreate_Default);
  P.Mode     := ePadMode_Simple;
  P.Layer    := eTopLayer;
  P.HoleSize := 0;
  P.X        := PX(X);
  P.Y        := PY(Y);
  P.TopXSize := MMsToCoord(W);
  P.TopYSize := MMsToCoord(H);
  P.TopShape := Shape;
  P.Name     := Name;
  AddObj(P);
End;

Procedure AddLine(X1, Y1, X2, Y2, W : Double; L : Integer);
Var
  T : IPCB_Track;
Begin
  T := PCBServer.PCBObjectFactory(eTrackObject, eNoDimension, eCreate_Default);
  T.X1 := PX(X1);  T.Y1 := PY(Y1);
  T.X2 := PX(X2);  T.Y2 := PY(Y2);
  T.Width := MMsToCoord(W);
  T.Layer := L;
  AddObj(T);
End;

Procedure Silk(X1, Y1, X2, Y2 : Double);
Begin
  AddLine(X1, Y1, X2, Y2, SILK_W, eTopOverlay);
End;

Procedure Dot(X, Y : Double);
Var
  A : IPCB_Arc;
Begin
  A := PCBServer.PCBObjectFactory(eArcObject, eNoDimension, eCreate_Default);
  A.XCenter    := PX(X);
  A.YCenter    := PY(Y);
  A.Radius     := MMsToCoord(0.05);
  A.LineWidth  := MMsToCoord(0.15);
  A.StartAngle := 0;
  A.EndAngle   := 360;
  A.Layer      := eTopOverlay;
  AddObj(A);
End;

Procedure Courtyard(X1, Y1, X2, Y2 : Double);
Begin
  AddLine(X1, Y1, X2, Y1, CRT_W, eMechanical15);
  AddLine(X2, Y1, X2, Y2, CRT_W, eMechanical15);
  AddLine(X2, Y2, X1, Y2, CRT_W, eMechanical15);
  AddLine(X1, Y2, X1, Y1, CRT_W, eMechanical15);
End;

Procedure AddStep(FileName : String);
Var
  B : IPCB_ComponentBody;
  M : IPCB_Model;
Begin
  If Not FileExists(MODEL_DIR + FileName) Then
  Begin
    ShowMessage('STEP model not found: ' + MODEL_DIR + FileName);
    Exit;
  End;
  Try
    B := PCBServer.PCBObjectFactory(eComponentBodyObject, eNoDimension, eCreate_Default);
    M := B.ModelFactory_FromFilename(MODEL_DIR + FileName, False);
    If M <> Nil Then
    Begin
      B.Model := M;
      B.SetState_FromModel;
      B.Layer := eMechanical13;
      AddObj(B);
    End;
  Except
    ShowMessage('Could not add 3D model ' + FileName + ' - place it manually (Place > 3D Body)');
  End;
End;

{ ---------------- footprints ---------------- }

Procedure Make_SOD323;
Begin
  If Not StartComp('SOD-323', 'SOD-323 diode, pads K (cathode, left) / A', 1.05) Then Exit;
  AddPad('K', -1.17, 0, 1.00, 0.70, eRectangular);
  AddPad('A',  1.17, 0, 1.00, 0.70, eRectangular);
  Silk(-0.85, -0.73, 0.90, -0.73);
  Silk(-0.85,  0.73, 0.90,  0.73);
  Silk( 0.90,  0.73, 0.90,  0.52);
  Silk( 0.90, -0.73, 0.90, -0.53);
  Silk(-0.45, -0.73, -0.45, 0.73);       { cathode bar }
  Courtyard(-1.92, -1.06, 1.92, 1.06);
  AddStep('SOD-323.step');
End;

Procedure Make_SOD123FL;
Begin
  If Not StartComp('SOD-123FL', 'SOD-123FL diode/TVS, pads K (cathode, left) / A', 1.00) Then Exit;
  AddPad('K', -1.65, 0, 1.30, 1.20, eRectangular);
  AddPad('A',  1.65, 0, 1.30, 1.20, eRectangular);
  Silk(-1.38, -0.88, 1.38, -0.88);
  Silk(-1.38,  0.88, 1.38,  0.88);
  Silk(-0.27, -0.51, -0.27, 0.51);       { cathode bar }
  Silk( 0.23, -0.51,  0.23, 0.51);
  Silk(-0.27,  0.00,  0.23, -0.51);
  Silk(-0.27,  0.00,  0.23,  0.51);
  Silk(-0.78,  0.00,  0.74,  0.00);
  Courtyard(-2.55, -1.20, 2.55, 1.20);
  AddStep('SOD-123FL.step');
End;

Procedure Make_SOT363;
Begin
  If Not StartComp('SOT-363', 'SOT-363 (SC-70-6), pin 1 bottom-left, 0.65mm pitch', 1.00) Then Exit;
  AddPad('1', -0.65, -0.80, 0.35, 0.90, eRectangular);
  AddPad('2',  0.00, -0.80, 0.35, 0.90, eRectangular);
  AddPad('3',  0.65, -0.80, 0.35, 0.90, eRectangular);
  AddPad('4',  0.65,  0.80, 0.35, 0.90, eRectangular);
  AddPad('5',  0.00,  0.80, 0.35, 0.90, eRectangular);
  AddPad('6', -0.65,  0.80, 0.35, 0.90, eRectangular);
  Silk( 1.06, -0.85,  1.24, -0.85);
  Silk( 1.24, -0.85,  1.24,  0.85);
  Silk( 1.24,  0.85,  1.06,  0.85);
  Silk(-1.06, -0.85, -1.24, -0.85);
  Silk(-1.24, -0.85, -1.24,  0.85);
  Silk(-1.24,  0.85, -1.06,  0.85);
  Dot(-1.14, -1.47);                     { pin 1 }
  Courtyard(-1.60, -1.50, 1.60, 1.50);
  AddStep('SOT-363.step');
End;

Procedure Make_SOT89;
Begin
  If Not StartComp('SOT-89-3', 'SOT-89-3, 1=OUT 2=GND(tab) 3=IN for 78L05 C20628877', 1.60) Then Exit;
  AddPad('1',  1.36, -1.50, 1.575, 0.672, eRectangular);
  AddPad('3',  1.36,  1.50, 1.575, 0.672, eRectangular);
  AddPad('2',  1.36,  0.00, 1.575, 0.660, eRectangular);   { centre lead }
  AddPad('2', -1.36,  0.00, 3.754, 1.900, eRounded);       { tab (obround) }
  Silk( 0.70, -2.02,  0.70, -2.34);
  Silk( 0.70, -0.51,  0.70, -0.98);
  Silk( 0.70,  0.98,  0.70,  0.51);
  Silk( 0.70,  2.34,  0.70,  2.02);
  Silk( 0.70,  2.34, -1.95,  2.34);
  Silk(-1.95,  2.34, -1.95,  1.17);
  Silk( 0.70, -2.34, -1.95, -2.34);
  Silk(-1.95, -2.34, -1.95, -1.17);
  Dot(2.45, -1.50);                      { pin 1 }
  Courtyard(-3.49, -2.60, 2.40, 2.60);
  AddStep('SOT-89-3.step');
End;

Procedure Make_SOD523;
Begin
  If Not StartComp('SOD-523', 'SOD-523 (SC-79) diode, pads K (cathode, left) / A', 0.70) Then Exit;
  AddPad('K', -0.65, 0, 0.60, 0.50, eRectangular);   { EasyEDA 0.60x0.36, taller for hand soldering }
  AddPad('A',  0.65, 0, 0.60, 0.50, eRectangular);
  Silk(-0.68, -0.48, 0.68, -0.48);
  Silk(-0.68,  0.48, 0.68,  0.48);
  Silk(-0.15, -0.30, -0.15, 0.30);       { cathode bar }
  Courtyard(-1.20, -0.80, 1.20, 0.80);
  AddStep('SOD-523.step');
End;

Procedure AddFootprints;
Begin
  Lib := PCBServer.GetCurrentPCBLibrary;
  If Lib = Nil Then
  Begin
    ShowMessage('Open resistive.PcbLib (and make it the active document) first.');
    Exit;
  End;
  Board := Lib.Board;

  PCBServer.PreProcess;
  Make_SOD323;
  Make_SOD123FL;
  Make_SOT363;
  Make_SOT89;
  Make_SOD523;
  PCBServer.PostProcess;

  Lib.Board.ViewManager_FullUpdate;
  ShowMessage('Done - check each footprint (and its 3D body) before use.');
End;
