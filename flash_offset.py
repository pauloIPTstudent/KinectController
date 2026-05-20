from SCons.Script import DefaultEnvironment

env = DefaultEnvironment()

def forcar_upload_loader(source, target, env):
    # Pega o caminho do binário que acabou de ser compilado
    firmware_bin = str(source[0])
    
    # Reconstrói do zero o comando de upload (UPLOADCMD)
    # Ignora os ficheiros de argumentos do CMake e força a escrita cirúrgica em 0x320000
    novo_comando = (
        '"$PYTHONEXE" "$UPLOADER" --chip esp32 --port "$UPLOAD_PORT" --baud $UPLOAD_SPEED '
        'write_flash -z 0x320000 "%s"' % firmware_bin
    )
    
    env.Replace(UPLOADCMD=novo_comando)
    print("\n>>> [SCRIPT OMNI] Comando de Upload reconstruído para gravar em 0x320000! <<<\n")

# Vincula a nossa substituição para rodar exatamente antes do upload disparar
env.AddPreAction("upload", forcar_upload_loader)