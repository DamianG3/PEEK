import React, { useState } from 'react'
import { supabase } from './supabaseClient'
import './index.css'

export default function RecargaPage() {
  const [tarjetaId, setTarjetaId] = useState('')
  const [tarjeta, setTarjeta]   = useState(null)
  const [viajes, setViajes]     = useState([])
  const [monto, setMonto]       = useState('')

  // Busca la tarjeta y sus viajes
  const buscarTarjeta = async () => {
    const id = parseInt(tarjetaId, 10)
    if (isNaN(id)) {
      return alert('ID inválido')
    }
    const { data: t, error: err1 } = await supabase
      .from('tarjetas')
      .select('nombre, saldo')
      .eq('id', id)
      .single()
    if (err1 || !t) {
      alert('Tarjeta no encontrada')
      setTarjeta(null)
      setViajes([])
      return
    }
    setTarjeta(t)

    const { data: v, error: err2 } = await supabase
      .from('viajes')
      .select('created_at')
      .eq('id_tarjeta', id)
      .order('created_at', { ascending: false })
    if (err2) console.error(err2)
    setViajes(v || [])
  }

  // Recarga el saldo de la tarjeta
  const recargarSaldo = async () => {
    const id = parseInt(tarjetaId, 10)
    const rec = parseFloat(monto)
    if (isNaN(rec) || rec <= 0) {
      return alert('Monto inválido')
    }
    const nuevoSaldo = tarjeta.saldo + rec
    const { error } = await supabase
      .from('tarjetas')
      .update({ saldo: nuevoSaldo })
      .eq('id', id)
    if (error) {
      return alert(error.message)
    }
    setTarjeta(prev => ({ ...prev, saldo: nuevoSaldo }))
    setMonto('')
    await buscarTarjeta()
  }

  return (
    <div className="container">
      <h1>PÉEK</h1>

      <div className="form-group">
        <label htmlFor="tarjetaId">ID:</label>
        <input
          id="tarjetaId"
          type="text"
          value={tarjetaId}
          onChange={(e) => setTarjetaId(e.target.value)}
        />
        <button onClick={buscarTarjeta}>Buscar</button>
      </div>

      {tarjeta && (
        <>
          <p><strong>Nombre:</strong> {tarjeta.nombre}</p>
          <p><strong>Saldo:</strong> ${tarjeta.saldo}</p>

          <div className="form-group">
            <label htmlFor="monto">Recargar:</label>
            <input
              id="monto"
              type="number"
              value={monto}
              onChange={(e) => setMonto(e.target.value)}
            />
            <button onClick={recargarSaldo}>Recargar</button>
          </div>

          <div>
            <h2>Viajes recientes</h2>
            <ul>
              {viajes.map((v) => (
                <li key={v.created_at}>
                  {new Date(v.created_at).toLocaleString()}
                </li>
              ))}
            </ul>
          </div>
        </>
      )}
    </div>
  )
}