import QtQuick
import QtQuick.Effects
import QtMultimedia

Rectangle {
	color: 'black'
	VideoOutput {
		id: videoOutput
		anchors.fill: parent
	}
	MultiEffect {
		source: videoOutput
		anchors.fill: videoOutput
	}
}
