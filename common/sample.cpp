#include <qendian.h>

#include "sample.h"

SampleConfigForm::SampleConfigForm(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);
}

SampleProtocol::SampleProtocol(StreamBase *stream, AbstractProtocol *parent)
    : AbstractProtocol(stream, parent)
{
    configForm = NULL;
}

SampleProtocol::~SampleProtocol()
{
    delete configForm;
}

AbstractProtocol* SampleProtocol::createInstance(StreamBase *stream,
    AbstractProtocol *parent)
{
    return new SampleProtocol(stream, parent);
}

quint32 SampleProtocol::protocolNumber() const
{
    return OstProto::Protocol::kSampleFieldNumber;
}

void SampleProtocol::protoDataCopyInto(OstProto::Protocol &protocol) const
{
    protocol.MutableExtension(OstProto::sample)->CopyFrom(data);
    protocol.mutable_protocol_id()->set_id(protocolNumber());
}

void SampleProtocol::protoDataCopyFrom(const OstProto::Protocol &protocol)
{
    if (protocol.protocol_id().id() == protocolNumber() &&
            protocol.HasExtension(OstProto::sample))
        data.MergeFrom(protocol.GetExtension(OstProto::sample));
}

QString SampleProtocol::name() const
{
    return QString("Sample Protocol");
}

QString SampleProtocol::shortName() const
{
    return QString("SAMPLE");
}

/*!
  Return the ProtocolIdType for your protocol \n

  If your protocol doesn't have a protocolId field, you don't need to 
  reimplement this method - the base class implementation will do the 
  right thing
*/
AbstractProtocol::ProtocolIdType SampleProtocol::protocolIdType() const
{
    return ProtocolIdIp;
}

/*!
  Return the protocolId for your protoocol based on the 'type' requested \n

  If not all types are valid for your protocol, handle the valid type(s) 
  and for the remaining fallback to the base class implementation; if your 
  protocol doesn't have a protocolId at all, you don't need to reimplement
  this method - the base class will do the right thing
*/
quint32 SampleProtocol::protocolId(ProtocolIdType type) const
{
    switch(type)
    {
        case ProtocolIdIp: return 1234;
        default:break;
    }

    return AbstractProtocol::protocolId(type);
}

int    SampleProtocol::fieldCount() const
{
    return sample_fieldCount;
}

AbstractProtocol::FieldFlags SampleProtocol::fieldFlags(int index) const
{
    AbstractProtocol::FieldFlags flags;

    flags = AbstractProtocol::fieldFlags(index);

    switch (index)
    {
        case sample_a:
        case sample_b:
        case sample_payloadLength:
            break;

        case sample_checksum:
            flags |= FieldIsCksum;
            break;

        case sample_x:
        case sample_y:
            break;

        case sample_is_override_checksum:
            flags |= FieldIsMeta;
            break;

        default:
            qFatal("%s: unimplemented case %d in switch", __PRETTY_FUNCTION__,
                index);
            break;
    }

    return flags;
}

QVariant SampleProtocol::fieldData(int index, FieldAttrib attrib,
        int streamIndex) const
{
    switch (index)
    {
        case sample_a:
        {
            int a = data.ab() >> 13;

            switch(attrib)
            {
                case FieldName:            
                    return QString("A");
                case FieldValue:
                    return a;
                case FieldTextValue:
                    return QString("%1").arg(a);
                case FieldFrameValue:
                    return QByteArray(1, (char) a);
                case FieldBitSize:
                    return 3;
                default:
                    break;
            }
            break;

        }
        case sample_b:
        {
            int b = data.ab() & 0x1FFF;

            switch(attrib)
            {
                case FieldName:            
                    return QString("B");
                case FieldValue:
                    return b;
                case FieldTextValue:
                    return QString("%1").arg(b, 4, BASE_HEX, QChar('0'));
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(2);
                    qToBigEndian((quint16) b, (uchar*) fv.data()); 
                    return fv;
                }
                case FieldBitSize:
                    return 13;
                default:
                    break;
            }
            break;
        }

        case sample_payloadLength:
        {
            switch(attrib)
            {
                case FieldName:            
                    return QString("Payload Length");
                case FieldValue:
                    return protocolFramePayloadSize(streamIndex);
                case FieldFrameValue:
                {
                    QByteArray fv;
                    int totlen;
                    totlen = protocolFramePayloadSize(streamIndex);
                    fv.resize(2);
                    qToBigEndian((quint16) totlen, (uchar*) fv.data());
                    return fv;
                }
                case FieldTextValue:
                    return QString("%1").arg(
                        protocolFramePayloadSize(streamIndex));
                case FieldBitSize:
                    return 16;
                default:
                    break;
            }
            break;
        }
        case sample_checksum:
        {
            quint16 cksum;

            switch(attrib)
            {
                case FieldValue:
                case FieldFrameValue:
                case FieldTextValue:
                {
                    if (data.is_override_checksum())
                        cksum = data.checksum();
                    else
                        cksum = protocolFrameCksum(streamIndex, CksumIp);
                }
                default:
                    cksum = 0; // avoid the 'maybe used unitialized' warning
                    break;
            }

            switch(attrib)
            {
                case FieldName:            
                    return QString("Checksum");
                case FieldValue:
                    return cksum;
                case FieldFrameValue:
                {
                    QByteArray fv;

                    fv.resize(2);
                    qToBigEndian(cksum, (uchar*) fv.data());
                    return fv;
                }
                case FieldTextValue:
                    return  QString("0x%1").arg(
                        cksum, 4, BASE_HEX, QChar('0'));;
                case FieldBitSize:
                    return 16;
                default:
                    break;
            }
            break;
        }
        case sample_x:
        {
            switch(attrib)
            {
                case FieldName:            
                    return QString("X");
                case FieldValue:
                    return data.x();
                case FieldTextValue:
                    return QString("%1").arg(data.x());
                    //return QString("%1").arg(data.x(), 8, BASE_HEX, QChar('0'));
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(4);
                    qToBigEndian((quint32) data.x(), (uchar*) fv.data());
                    return fv;
                }
                default:
                    break;
            }
            break;
        }
        case sample_y:
        {
            switch(attrib)
            {
                case FieldName:            
                    return QString("Y");
                case FieldValue:
                    return data.y();
                case FieldTextValue:
                    //return QString("%1").arg(data.y());
                    return QString("%1").arg(data.y(), 8, BASE_HEX, QChar('0'));
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(4);
                    qToBigEndian((quint32) data.y(), (uchar*) fv.data());
                    return fv;
                }
                default:
                    break;
            }
            break;
        }


        // Meta fields
        case sample_is_override_checksum:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.is_override_checksum();
                default:
                    break;
            }
            break;
        }
        default:
            qFatal("%s: unimplemented case %d in switch", __PRETTY_FUNCTION__,
                index);
            break;
    }

    return AbstractProtocol::fieldData(index, attrib, streamIndex);
}

bool SampleProtocol::setFieldData(int index, const QVariant &value, 
        FieldAttrib attrib)
{
    bool isOk = false;

    if (attrib != FieldValue)
        goto _exit;

    switch (index)
    {
        case sample_a:
        {
            uint a = value.toUInt(&isOk);
            if (isOk)
                data.set_ab((data.ab() & 0xe000) | (a << 13));
            break;
        }
        case sample_b:
        {
            uint b = value.toUInt(&isOk);
            if (isOk)
                data.set_ab((data.ab() & 0x1FFF) | b);
            break;
        }
        case sample_payloadLength:
        {
            uint len = value.toUInt(&isOk);
            if (isOk)
                data.set_payload_length(len);
            break;
        }
        case sample_checksum:
        {
            uint csum = value.toUInt(&isOk);
            if (isOk)
                data.set_checksum(csum);
            break;
        }
        case sample_x:
        {
            uint x = value.toUInt(&isOk);
            if (isOk)
                data.set_x(x);
            break;
        }
        case sample_y:
        {
            uint y = value.toUInt(&isOk);
            if (isOk)
                data.set_y(y);
            break;
        }
        case sample_is_override_checksum:
        {
            bool ovr = value.toBool();
            data.set_is_override_checksum(ovr);
            isOk = true;
            break;
        }
        default:
            qFatal("%s: unimplemented case %d in switch", __PRETTY_FUNCTION__,
                index);
            break;
    }

_exit:
    return isOk;
}

/*!
  Return the protocol frame size in bytes\n

  If your protocol has a fixed size - you don't need to reimplement this; the
  base class implementation is good enough
*/
int SampleProtocol::protocolFrameSize(int streamIndex) const
{
    return AbstractProtocol::protocolFrameSize(streamIndex);
}

/*!
  If your protocol has any variable fields, return true \n

  Otherwise you don't need to reimplement this method - the base class always
  returns false
*/
bool SampleProtocol::isProtocolFrameValueVariable() const
{
    return false;
}

/*!
  If your protocol frame size can vary across pkts of the same stream,
  return true \n

  Otherwise you don't need to reimplement this method - the base class always
  returns false
*/
bool SampleProtocol::isProtocolFrameSizeVariable() const
{
    return false;
}

QWidget* SampleProtocol::configWidget()
{
    if (configForm == NULL)
    {
        configForm = new SampleConfigForm;
        loadConfigWidget();
    }

    return configForm;
}

void SampleProtocol::loadConfigWidget()
{
    configWidget();

    configForm->sampleA->setText(fieldData(sample_a, FieldValue).toString());
    configForm->sampleB->setText(fieldData(sample_b, FieldValue).toString());

    configForm->samplePayloadLength->setText(
        fieldData(sample_payloadLength, FieldValue).toString());

    configForm->isChecksumOverride->setChecked(
        fieldData(sample_is_override_checksum, FieldValue).toBool());
    configForm->sampleChecksum->setText(uintToHexStr(
        fieldData(sample_checksum, FieldValue).toUInt(), 2));

    configForm->sampleX->setText(fieldData(sample_x, FieldValue).toString());
    configForm->sampleY->setText(fieldData(sample_y, FieldValue).toString());

}

void SampleProtocol::storeConfigWidget()
{
    bool isOk;

    configWidget();
    setFieldData(sample_a, configForm->sampleA->text());
    setFieldData(sample_b, configForm->sampleB->text());

    setFieldData(sample_payloadLength, configForm->samplePayloadLength->text());
    setFieldData(sample_is_override_checksum, 
        configForm->isChecksumOverride->isChecked());
    setFieldData(sample_checksum, configForm->sampleChecksum->text().toUInt(&isOk, BASE_HEX));

    setFieldData(sample_x, configForm->sampleX->text());
    setFieldData(sample_y, configForm->sampleY->text());
}
