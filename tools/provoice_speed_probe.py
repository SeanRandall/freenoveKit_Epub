"""Render a ProVoice voice at a requested speed using the 32-bit engine."""

import argparse
import ctypes
import pathlib
import wave


class WaveFormatEx(ctypes.Structure):
    _fields_ = [
        ("format_tag", ctypes.c_ushort),
        ("channels", ctypes.c_ushort),
        ("samples_per_sec", ctypes.c_uint32),
        ("avg_bytes_per_sec", ctypes.c_uint32),
        ("block_align", ctypes.c_ushort),
        ("bits_per_sample", ctypes.c_ushort),
        ("extra_size", ctypes.c_ushort),
    ]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("engine_dir", type=pathlib.Path)
    parser.add_argument("voice")
    parser.add_argument("output", type=pathlib.Path)
    parser.add_argument("--speed", type=int, default=15)
    parser.add_argument("--pitch", type=int, default=5)
    parser.add_argument("--sentence-pause", type=int, default=0)
    parser.add_argument("--text", required=True)
    args = parser.parse_args()

    print("loading engine", flush=True)
    dll = ctypes.WinDLL(str(args.engine_dir / "FBVTV32.DLL"))
    dll.OpenSpeech.argtypes = [ctypes.c_void_p, ctypes.c_ushort, ctypes.c_char_p]
    dll.OpenSpeech.restype = ctypes.c_void_p
    dll.CloseSpeech.argtypes = [ctypes.c_void_p]
    dll.SpeechStatus.argtypes = [ctypes.c_void_p]
    dll.SpeechStatus.restype = ctypes.c_int
    dll.SetSpeechParameter.argtypes = [ctypes.c_void_p, ctypes.c_ushort, ctypes.c_long]
    dll.SetSpeechParameter.restype = ctypes.c_int
    dll.NextSentence.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
    dll.NextSentence.restype = ctypes.c_void_p
    dll.TextToPhonetics.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_ushort]
    dll.TextToPhonetics.restype = ctypes.c_void_p
    dll.FreePhoneticsBuffer.argtypes = [ctypes.c_void_p]
    dll.OpenBackend.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
    dll.OpenBackend.restype = ctypes.c_long
    dll.GetPCMdata.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_long]
    dll.GetPCMdata.restype = ctypes.c_long
    dll.CloseBackend.argtypes = [ctypes.c_void_p]
    dll.GetFontWaveFormat.argtypes = [ctypes.c_void_p, ctypes.POINTER(WaveFormatEx)]
    dll.GetFontWaveFormat.restype = ctypes.c_int

    voice = ctypes.create_string_buffer(args.voice.encode("ascii"))
    print("opening voice", flush=True)
    scb = dll.OpenSpeech(None, 8, voice)
    status = dll.SpeechStatus(scb) if scb else -112
    if status < 0 and status != -200:
        raise RuntimeError(f"cannot open {args.voice}: {status}")

    try:
        print("setting parameters", flush=True)
        for param, value in ((3, args.speed), (2, args.pitch), (22, args.sentence_pause)):
            result = dll.SetSpeechParameter(scb, param, value)
            if result:
                raise RuntimeError(f"parameter {param}={value} failed: {result}")

        fmt = WaveFormatEx()
        dll.GetFontWaveFormat(scb, ctypes.byref(fmt))
        text = ctypes.create_string_buffer(args.text.encode("cp1252"))
        pcm = bytearray()
        print("rendering", flush=True)
        sentence = dll.NextSentence(scb, text)
        block = ctypes.create_string_buffer(32768)
        while sentence:
            phonetics = dll.TextToPhonetics(scb, ctypes.cast(sentence, ctypes.c_char_p), 0)
            if phonetics:
                result = dll.OpenBackend(scb, phonetics)
                if result:
                    raise RuntimeError(f"OpenBackend failed: {result}")
                while True:
                    count = dll.GetPCMdata(scb, block, len(block))
                    if count <= 0:
                        break
                    pcm.extend(block.raw[:count])
                dll.CloseBackend(scb)
                dll.FreePhoneticsBuffer(phonetics)
            sentence = dll.NextSentence(scb, None)
        print("writing wave", flush=True)

        args.output.parent.mkdir(parents=True, exist_ok=True)
        with wave.open(str(args.output), "wb") as output:
            output.setnchannels(fmt.channels)
            output.setsampwidth(fmt.bits_per_sample // 8)
            output.setframerate(fmt.samples_per_sec)
            output.writeframes(pcm)
        seconds = len(pcm) / fmt.avg_bytes_per_sec
        words = len(args.text.split())
        print(f"{args.voice}: {words} words, {seconds:.3f} seconds, {words * 60 / seconds:.1f} WPM")
        print(args.output)
    finally:
        dll.CloseSpeech(scb)


if __name__ == "__main__":
    main()
