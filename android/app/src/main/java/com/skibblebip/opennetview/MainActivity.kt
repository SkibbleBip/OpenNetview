package com.skibblebip.opennetview

import android.app.Activity
import android.graphics.Color
import android.media.MediaCodec
import android.media.MediaFormat
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.Gravity
import android.view.KeyEvent
import android.view.MotionEvent
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.ViewGroup
import android.widget.Button
import android.widget.EditText
import android.widget.LinearLayout
import android.widget.TextView
import java.io.ByteArrayOutputStream
import java.io.DataOutputStream
import java.net.DatagramPacket
import java.net.DatagramSocket
import java.net.InetSocketAddress
import java.net.Socket
import java.nio.charset.StandardCharsets
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors
import java.util.concurrent.atomic.AtomicBoolean

/** Android equivalent of the desktop client's connect, video, and camera-control UI. */
class MainActivity : Activity(), SurfaceHolder.Callback {
    private val ui = Handler(Looper.getMainLooper())
    private val commandExecutor: ExecutorService = Executors.newSingleThreadExecutor()
    private lateinit var addressInput: EditText
    private lateinit var status: TextView
    private lateinit var surfaceView: SurfaceView
    @Volatile private var commandSocket: Socket? = null
    @Volatile private var output: DataOutputStream? = null
    @Volatile private var connectedHost: String? = null
    @Volatile private var receiver: RtpVideoReceiver? = null
    @Volatile private var connecting = false
    private var videoSurface: Surface? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.statusBarColor = Color.TRANSPARENT
        window.navigationBarColor = Color.TRANSPARENT
        window.decorView.systemUiVisibility = (
            android.view.View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                or android.view.View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                or android.view.View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                or android.view.View.SYSTEM_UI_FLAG_FULLSCREEN
                or android.view.View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                or android.view.View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
            )
        buildUi()
    }

    private fun buildUi() {
        // The video is the base layer. All controls are added afterwards as
        // transparent overlays so the camera image remains visible beneath them.
        val root = android.widget.FrameLayout(this).apply {
            setBackgroundColor(Color.BLACK)
            clipChildren = true
            clipToPadding = true
        }

        surfaceView = SurfaceView(this)
        surfaceView.holder.addCallback(this)
        root.addView(surfaceView, android.widget.FrameLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.MATCH_PARENT
        ))

        val overlay = android.widget.FrameLayout(this)
        root.addView(overlay, android.widget.FrameLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.MATCH_PARENT
        ))

        val connectRow = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            setPadding(8, 4, 8, 4)
            setBackgroundColor(0x99000000.toInt())
        }
        addressInput = EditText(this).apply {
            hint = "Camera IP / hostname"
            setSingleLine(true)
            setTextColor(Color.WHITE)
            setHintTextColor(Color.LTGRAY)
            setPadding(12, 4, 12, 4)
            setText("192.168.1.1")
            textSize = 14f
            backgroundTintList = android.content.res.ColorStateList.valueOf(Color.LTGRAY)
        }
        connectRow.addView(addressInput, LinearLayout.LayoutParams(220, 48))
        val connectButton = Button(this).apply {
            text = "Connect"
            setOnClickListener { connect() }
        }
        connectRow.addView(connectButton)
        val connectParams = android.widget.FrameLayout.LayoutParams(
            ViewGroup.LayoutParams.WRAP_CONTENT,
            ViewGroup.LayoutParams.WRAP_CONTENT,
            Gravity.TOP or Gravity.START
        ).apply { setMargins(8, 8, 8, 8) }
        overlay.addView(connectRow, connectParams)

        status = TextView(this).apply {
            text = "Enter the camera address and connect."
            setTextColor(Color.WHITE)
            textSize = 13f
            setPadding(10, 7, 10, 7)
            setBackgroundColor(0x99000000.toInt())
        }
        val statusParams = android.widget.FrameLayout.LayoutParams(
            ViewGroup.LayoutParams.WRAP_CONTENT,
            ViewGroup.LayoutParams.WRAP_CONTENT,
            Gravity.BOTTOM or Gravity.START
        ).apply { setMargins(8, 8, 8, 8) }
        overlay.addView(status, statusParams)

        val controls = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            setPadding(8, 6, 8, 6)
            setBackgroundColor(0x99000000.toInt())
        }
        val pad = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.CENTER
        }
        pad.addView(directionButton("▲", 0, -MOVE_STEPS))
        val middle = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER
        }
        middle.addView(directionButton("◀", MOVE_STEPS, 0))
        middle.addView(directionButton("▼", 0, MOVE_STEPS))
        middle.addView(directionButton("▶", -MOVE_STEPS, 0))
        pad.addView(middle)
        controls.addView(pad)
        val home = Button(this).apply {
            text = "Home"
            setOnClickListener { sendCommand("goto 0 0 ") }
        }
        controls.addView(home)
        val controlsParams = android.widget.FrameLayout.LayoutParams(
            ViewGroup.LayoutParams.WRAP_CONTENT,
            ViewGroup.LayoutParams.WRAP_CONTENT,
            Gravity.BOTTOM or Gravity.END
        ).apply { setMargins(8, 8, 8, 8) }
        overlay.addView(controls, controlsParams)

        setContentView(root)
    }

    private fun directionButton(label: String, x: Int, y: Int): Button {
        val button = Button(this).apply { text = label; contentDescription = label }
        val repeat = object : Runnable {
            override fun run() {
                if (button.isPressed && connectedHost != null) {
                    sendCommand("move $x $y ")
                    ui.postDelayed(this, MOVE_INTERVAL_MS.toLong())
                }
            }
        }
        button.setOnTouchListener { view, event ->
            when (event.action) {
                MotionEvent.ACTION_DOWN -> {
                    view.isPressed = true
                    sendCommand("move $x $y ")
                    ui.postDelayed(repeat, MOVE_INTERVAL_MS.toLong())
                    true
                }
                MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                    view.isPressed = false
                    ui.removeCallbacks(repeat)
                    true
                }
                else -> true
            }
        }
        return button
    }

    private fun connect() {
        if (connecting || connectedHost != null) return
        val host = addressInput.text.toString().trim()
        if (host.isEmpty()) {
            setStatus("Enter a camera IP address or hostname.", Color.YELLOW)
            return
        }
        connecting = true
        setStatus("Connecting to $host:52000…", Color.YELLOW)
        Thread {
            try {
                val socket = Socket()
                socket.tcpNoDelay = true
                socket.connect(InetSocketAddress(host, 52000), 5000)
                commandSocket = socket
                output = DataOutputStream(socket.getOutputStream())
                connectedHost = host
                ui.post {
                    connecting = false
                    addressInput.isEnabled = false
                    setStatus("Connected to $host · RTP/H.264 UDP 6969 · TCP 52000", Color.GREEN)
                    startReceiverIfReady()
                }
            } catch (e: Exception) {
                connecting = false
                closeConnection()
                ui.post { setStatus("Connection failed: ${e.message ?: e.javaClass.simpleName}", Color.RED) }
            }
        }.start()
    }

    private fun sendCommand(text: String) {
        commandExecutor.execute {
            try {
                val stream = output ?: return@execute
                val payload = text.toByteArray(StandardCharsets.US_ASCII)
                require(payload.size <= MAX_PAYLOAD_SIZE) { "Command is too long" }
                val padded = ByteArray(MAX_PAYLOAD_SIZE)
                System.arraycopy(payload, 0, padded, 0, payload.size)
                val crc = crc16(padded)
                synchronized(stream) {
                    stream.writeByte(START_COMMAND)
                    stream.write(padded)
                    stream.writeShort(crc)
                    stream.flush()
                }
            } catch (e: Exception) {
                ui.post { setStatus("Command send failed: ${e.message ?: "connection lost"}", Color.RED) }
            }
        }
    }

    private fun startReceiverIfReady() {
        val surface = videoSurface ?: return
        val host = connectedHost ?: return
        if (receiver != null) return
        val video = RtpVideoReceiver(surface) { message ->
            ui.post { if (connectedHost != null) setStatus(message, Color.LTGRAY) }
        }
        receiver = video
        video.start()
        setStatus("Connected to $host · waiting for H.264 video…", Color.GREEN)
    }

    private fun setStatus(message: String, color: Int) {
        if (::status.isInitialized) {
            status.text = message
            status.setTextColor(color)
        }
    }

    private fun closeConnection() {
        receiver?.stop()
        receiver = null
        try { output?.close() } catch (_: Exception) { }
        try { commandSocket?.close() } catch (_: Exception) { }
        output = null
        commandSocket = null
        connectedHost = null
        connecting = false
        ui.post { if (::addressInput.isInitialized) addressInput.isEnabled = true }
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        videoSurface = holder.surface
        startReceiverIfReady()
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) = Unit

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        videoSurface = null
        receiver?.stop()
        receiver = null
    }

    override fun onKeyDown(keyCode: Int, event: KeyEvent): Boolean {
        if (event.repeatCount == 0) {
            when (keyCode) {
                KeyEvent.KEYCODE_DPAD_LEFT, KeyEvent.KEYCODE_A -> sendCommand("move $MOVE_STEPS 0 ")
                KeyEvent.KEYCODE_DPAD_RIGHT, KeyEvent.KEYCODE_D -> sendCommand("move -$MOVE_STEPS 0 ")
                KeyEvent.KEYCODE_DPAD_UP, KeyEvent.KEYCODE_W -> sendCommand("move 0 -$MOVE_STEPS ")
                KeyEvent.KEYCODE_DPAD_DOWN, KeyEvent.KEYCODE_S -> sendCommand("move 0 $MOVE_STEPS ")
                KeyEvent.KEYCODE_HOME -> sendCommand("goto 0 0 ")
                KeyEvent.KEYCODE_ESCAPE, KeyEvent.KEYCODE_BACK -> { closeConnection(); return true }
                else -> return super.onKeyDown(keyCode, event)
            }
            return true
        }
        return super.onKeyDown(keyCode, event)
    }

    override fun onDestroy() {
        closeConnection()
        commandExecutor.shutdownNow()
        super.onDestroy()
    }

    private class RtpVideoReceiver(
        private val surface: Surface,
        private val report: (String) -> Unit
    ) {
        private val running = AtomicBoolean(false)
        @Volatile private var socket: DatagramSocket? = null
        private var worker: Thread? = null

        fun start() {
            if (!running.compareAndSet(false, true)) return
            worker = Thread({ receiveAndDecode() }, "OpenNetview-RTP").apply { start() }
        }

        fun stop() {
            running.set(false)
            socket?.close()
            worker?.interrupt()
        }

        private fun receiveAndDecode() {
            var decoder: MediaCodec? = null
            try {
                val format = MediaFormat.createVideoFormat(MediaFormat.MIMETYPE_VIDEO_AVC, 1280, 720)
                format.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, 2 * 1024 * 1024)
                decoder = MediaCodec.createDecoderByType(MediaFormat.MIMETYPE_VIDEO_AVC)
                decoder.configure(format, surface, null, 0)
                decoder.start()
                // Fill the display without distorting the camera's aspect ratio.
                // This crops the longer dimension when the screen and stream differ.
                decoder.setVideoScalingMode(MediaCodec.VIDEO_SCALING_MODE_SCALE_TO_FIT_WITH_CROPPING)
                val udp = DatagramSocket(null)
                socket = udp
                udp.reuseAddress = true
                udp.bind(InetSocketAddress(6969))
                udp.soTimeout = 500
                report("Video receiver active · UDP 6969")

                val packetData = ByteArray(65535)
                val accessUnit = ByteArrayOutputStream()
                var fuOpen = false
                val info = MediaCodec.BufferInfo()
                while (running.get()) {
                    try {
                        val packet = DatagramPacket(packetData, packetData.size)
                        udp.receive(packet)
                        val offset = rtpPayloadOffset(packet.data, packet.length)
                        if (offset >= 0) {
                            val payloadType = packet.data[1].toInt() and 0x7f
                            if (payloadType == 96 && offset < packet.length) {
                                val end = packet.data[1].toInt() and 0x80 != 0
                                fuOpen = appendH264(packet.data, offset, packet.length, accessUnit, fuOpen)
                                if (end && accessUnit.size() > 0 && !fuOpen) {
                                    queueAccessUnit(decoder, accessUnit.toByteArray())
                                    accessUnit.reset()
                                }
                            }
                        }
                    } catch (_: java.net.SocketTimeoutException) {
                        // Periodically drain decoded output even if the stream pauses.
                    }
                    drainOutput(decoder, info)
                }
            } catch (e: Exception) {
                if (running.get()) report("Video error: ${e.message ?: e.javaClass.simpleName}")
            } finally {
                running.set(false)
                try { socket?.close() } catch (_: Exception) { }
                try { decoder?.stop() } catch (_: Exception) { }
                try { decoder?.release() } catch (_: Exception) { }
            }
        }

        private fun queueAccessUnit(codec: MediaCodec, bytes: ByteArray) {
            if (bytes.isEmpty()) return
            val index = codec.dequeueInputBuffer(10_000)
            if (index < 0) return
            val input = codec.getInputBuffer(index) ?: return
            if (bytes.size > input.capacity()) return
            input.clear()
            input.put(bytes)
            codec.queueInputBuffer(index, 0, bytes.size, System.nanoTime() / 1000L, 0)
        }

        private fun drainOutput(codec: MediaCodec, info: MediaCodec.BufferInfo) {
            while (true) {
                val index = codec.dequeueOutputBuffer(info, 0)
                if (index < 0) break
                codec.releaseOutputBuffer(index, true)
            }
        }

        /** Return the byte offset after the variable-length RTP header, or -1 if invalid. */
        private fun rtpPayloadOffset(data: ByteArray, length: Int): Int {
            if (length < 12 || (data[0].toInt() and 0xc0) != 0x80) return -1
            var offset = 12 + (data[0].toInt() and 0x0f) * 4
            if (offset > length) return -1
            if (data[0].toInt() and 0x10 != 0) {
                if (offset + 4 > length) return -1
                val extensionWords = ((data[offset + 2].toInt() and 0xff) shl 8) or
                    (data[offset + 3].toInt() and 0xff)
                offset += 4 + extensionWords * 4
            }
            val padding = data[0].toInt() and 0x20 != 0
            if (offset > length || (padding && (data[length - 1].toInt() and 0xff) > length - offset)) return -1
            return offset
        }

        /** Append RTP H.264 single-NAL, STAP-A, or FU-A payloads as Annex-B NAL units. */
        private fun appendH264(data: ByteArray, offset: Int, length: Int, au: ByteArrayOutputStream, fuWasOpen: Boolean): Boolean {
            val payloadEnd = if (data[0].toInt() and 0x20 != 0) length - (data[length - 1].toInt() and 0xff) else length
            if (offset >= payloadEnd) return false
            val type = data[offset].toInt() and 0x1f
            var fuOpen = fuWasOpen
            when {
                type in 1..23 -> {
                    writeStartCode(au)
                    au.write(data, offset, payloadEnd - offset)
                    fuOpen = false
                }
                type == 24 -> {
                    var cursor = offset + 1
                    while (cursor + 2 <= payloadEnd) {
                        val size = ((data[cursor].toInt() and 0xff) shl 8) or (data[cursor + 1].toInt() and 0xff)
                        cursor += 2
                        if (size <= 0 || cursor + size > payloadEnd) break
                        writeStartCode(au)
                        au.write(data, cursor, size)
                        cursor += size
                    }
                    fuOpen = false
                }
                type == 28 && offset + 2 < payloadEnd -> {
                    val fuHeader = data[offset + 1].toInt() and 0xff
                    val start = fuHeader and 0x80 != 0
                    val end = fuHeader and 0x40 != 0
                    if (start) {
                        writeStartCode(au)
                        au.write((data[offset].toInt() and 0xe0) or (fuHeader and 0x1f))
                        au.write(data, offset + 2, payloadEnd - offset - 2)
                        fuOpen = !end
                    } else if (fuOpen) {
                        au.write(data, offset + 2, payloadEnd - offset - 2)
                        if (end) fuOpen = false
                    }
                }
            }
            return fuOpen
        }

        private fun writeStartCode(out: ByteArrayOutputStream) {
            out.write(byteArrayOf(0, 0, 0, 1))
        }
    }

    companion object {
        private const val START_COMMAND = 0x02
        private const val MAX_PAYLOAD_SIZE = 16
        private const val MOVE_STEPS = 10
        private const val MOVE_INTERVAL_MS = 25

        /** CRC-16/CCITT-FALSE as used by the firmware (poly 0x1021, init 0xffff). */
        private fun crc16(data: ByteArray): Int {
            var crc = 0xffff
            for (byte in data) {
                crc = crc xor ((byte.toInt() and 0xff) shl 8)
                repeat(8) {
                    crc = if (crc and 0x8000 != 0) ((crc shl 1) xor 0x1021) and 0xffff
                    else (crc shl 1) and 0xffff
                }
            }
            return crc
        }
    }
}
