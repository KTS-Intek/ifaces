#include "conf2modemhelper.h"

QVariantHash conf2modemHelper::aboutZigBeeModem2humanReadable(const QVariantHash &aboutModem)
{
    QVariantHash h;
    bool ok;
    int val;
    //"ATVR" "0206"
    if(aboutModem.contains("ATVR") && aboutModem.value("ATVR").toString().length() > 3)
        h.insert("VR", aboutModem.value("ATVR").toString());

    //"ATSL" "007A8E23"
    //"ATSH" "000D6F00"
    if(aboutModem.contains("ATSL") && aboutModem.contains("ATSH")
            && aboutModem.value("ATSL").toString().length() == 8 && aboutModem.value("ATSH").toString().length() == 8
            && aboutModem.value("ATSL").toString().toULongLong(&ok, 16) > 0 && ok
            && aboutModem.value("ATSH").toString().toULongLong(&ok, 16) > 0 && ok)
        h.insert("SN (EUI64)", aboutModem.value("ATSH").toString() + aboutModem.value("ATSL").toString());

    //"ATDB" "000 RSSI:-42 LQI:255"
    if(aboutModem.contains("ATDB") && aboutModem.value("ATDB").toString().contains("RSSI:") && aboutModem.value("ATDB").toString().contains("LQI:")){
        QString s = aboutModem.value("ATDB").toString();
        s = s.left( s.indexOf("LQI:") + 7 );
        QStringList l = s.split("LQI:", QString::SkipEmptyParts);
        s = l.first();
        s = s.mid(s.indexOf("RSSI:") + 5).simplified().trimmed();
        val = s.toInt(&ok);
        if(val > (-99) && val < (-1) && ok)
            h.insert("RSSI", QString("%1 dBm").arg(val) );
        val = l.last().toInt(&ok);
        if(val >= 0 && val < 256 && ok)
            h.insert("LQI", QString::number(val));
    }

    if(aboutModem.contains("ATAD") && QString("C R E M").split(" ").contains(aboutModem.value("ATAD").toString().toUpper())){
        switch(QString("C R E M").split(" ").indexOf(aboutModem.value("ATAD").toString().toUpper())){
        case 0: h.insert("Type", "Coordinator"); break;
        case 1: h.insert("Type", "Router"); break;
        case 2: h.insert("Type", "End Device"); break;
        case 3: h.insert("Type", "Mobile Device"); break;
        }
    }

    if(aboutModem.contains("ATHP") && aboutModem.value("ATHP").toString().toInt(&ok, 16) > 0 && ok)
        h.insert("Hops", QString::number(aboutModem.value("ATHP").toString().toInt(&ok, 16)));

    if(aboutModem.contains("ATID") && aboutModem.value("ATID").toString().toInt(&ok, 16) >= 0 && ok && aboutModem.value("ATID").toString().toInt(&ok, 16) <= 0x3FFF)
        h.insert("Network ID", "0x" + aboutModem.value("ATID").toString() );


    if(aboutModem.contains("ATHV") && aboutModem.value("ATHV").toString().length() > 3)
        h.insert("HV", aboutModem.value("ATHV").toString());

    if(aboutModem.contains("ATCH") && aboutModem.value("ATCH").toString().toInt(&ok, 16) >= 0x0B && ok && aboutModem.value("ATCH").toString().toInt(&ok, 16) <= 0x1A){
        QStringList listChannels = QString("11 | 2405MHz | 0x0B\n12 | 2410MHz | 0x0C\n13 | 2415MHz | 0x0D\n14 | 2420MHz | 0x0E\n15 | 2425MHz | 0x0F\n"
                                           "16 | 2430MHz | 0x10\n17 | 2435MHz | 0x11\n18 | 2440MHz | 0x12\n19 | 2445MHz | 0x13\n20 | 2450MHz | 0x14\n"
                                           "21 | 2455MHz | 0x15\n22 | 2460MHz | 0x16\n23 | 2465MHz | 0x17\n24 | 2470MHz | 0x18\n25 | 2475MHz | 0x19\n"
                                           "26 | 2480MHz | 0x1A").split("\n");
        val = aboutModem.value("ATCH").toString().toInt(&ok, 16) - 0x0B;
        if(val >= 0 && val < listChannels.size() && ok)
            h.insert("Channel", listChannels.at(val));
    }

    if(aboutModem.contains("ATPL") && aboutModem.value("ATPL").toString().toInt(&ok) >= (-43) && ok && aboutModem.value("ATPL").toString().toInt(&ok) <= 3){
        QStringList listPowerLevel ;

        val = h.value("HV").toString().mid(1).toInt(&ok);
        if(ok){
            if((val%2) > 0 )     //1 100mW  or  0 2mW
                listPowerLevel = QString(" 003 dBm | 100.0 mW MAX\n 002 dBm |  80.0 mW\n 001 dBm |  63.0 mW\n 000 dBm |  50.0 mW\n"
                    "-001 dBm | 40.0 mW\n-002 dBm | 32.0 mW\n-003 dBm | 25.0 mW\n-004 dBm | 20.0 mW\n"
                    "-005 dBm | 16.0 mW\n-006 dBm | 12.5 mW\n-007 dBm | 10.0 mW\n-008 dBm |  8.1 mW\n"
                    "-009 dBm |  7.9 mW\n-011 dBm |  5.0 mW\n-012 dBm |  4.0 mW\n-014 dBm |  2.5 mW\n"
                    "-017 dBm |  1.0 mW\n-020 dBm |  630 uW\n-026 dBm |  160 uW\n-043 dBm |  3 uW MIN").split("\n");
            else
                listPowerLevel = QString(" 003 dBm | 2.00 mW MAX\n 002 dBm | 1.60 mW\n 001 dBm | 1.30 mW\n 000 dBm | 1.00 mW\n"
                    "-001 dBm | 0.79 mW\n-002 dBm | 0.63 mW\n-003 dBm | 0.50 mW\n-004 dBm | 0.39 mW\n"
                    "-005 dBm | 0.31 mW\n-006 dBm | 0.25 mW\n-007 dBm | 0.20 mW\n-008 dBm | 0.16 mW\n"
                    "-009 dBm | 0.13 mW\n-011 dBm | 0.08 mW\n-012 dBm | 0.06 mW\n-014 dBm | 0.04 mW\n"
                    "-017 dBm | 0.02 mW\n-020 dBm | 10 uW\n-026 dBm | 2.5 uW\n-043 dBm | 50nW MIN").split("\n");
        }else{
            listPowerLevel = QString(" 003 dBm | 100.0 (2.00) mW MAX\n 002 dBm |  80.0 (1.60) mW\n 001 dBm |  63.0 (1.30) mW\n 000 dBm |  50.0 (1.00) mW\n"
                                                         "-001 dBm | 40.0 (0.79) mW\n-002 dBm | 32.0 (0.63) mW\n-003 dBm | 25.0 (0.50) mW\n-004 dBm | 20.0 (0.39) mW\n"
                                                         "-005 dBm | 16.0 (0.31) mW\n-006 dBm | 12.5 (0.25) mW\n-007 dBm | 10.0 (0.20) mW\n-008 dBm |  8.1 (0.16) mW\n"
                                                         "-009 dBm |  7.9 (0.13) mW\n-011 dBm |  5.0 (0.08) mW\n-012 dBm |  4.0 (0.06) mW\n-014 dBm |  2.5 (0.04) mW\n"
                                                         "-017 dBm |  1.0 (0.02) mW\n-020 dBm |  630 (10) uW\n-026 dBm |  160 (2.5) uW\n-043 dBm |  3 uW (50nW) MIN").split("\n");
        }



        int power = aboutModem.value("ATPL").toString().toInt(&ok, 10);

        if(ok){
            if(power > (-44) && power < (-26)) //-43...-25  pl -43
                power = 19;
            else{
                if(power > (-27) && power < (-20)){ //-26...-21
                    power = 18;
                }else{
                    switch(power){
                    case   3: { power =  0; break; }
                    case   2: { power =  1; break; }
                    case   1: { power =  2; break; }
                    case   0: { power =  3; break; }
                    case  -1: { power =  4; break; }
                    case  -2: { power =  5; break; }
                    case  -3: { power =  6; break; }
                    case  -4: { power =  7; break; }
                    case  -5: { power =  8; break; }
                    case  -6: { power =  9; break; }
                    case  -7: { power = 10; break; }
                    case  -8: { power = 11; break; }
                    case  -9: { power = 12; break; }
                    case -10: { power = 13; break; }
                    case -11: { power = 13; break; }
                    case -12: { power = 14; break; }
                    case -13: { power = 15; break; }
                    case -14: { power = 15; break; }
                    case -15: { power = 16; break; }
                    case -16: { power = 16; break; }
                    case -17: { power = 16; break; }
                    case -18: { power = 17; break; }
                    case -19: { power = 17; break; }
                    case -20: { power = 17; break; }
                    default : { power = -1; break; }
                    }
                }
            }
        }

        if(ok && power >= 0 && power < listPowerLevel.size())
            h.insert("Power level", listPowerLevel.at(power).trimmed());
    }


    return h;
}
