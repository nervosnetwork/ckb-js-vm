use ckb_js_tests::read_tx_template;

pub fn main() -> Result<(), Box<dyn std::error::Error>> {
    let tx = read_tx_template("templates/default_by_cell.json")?;

    let json = serde_json::to_string_pretty(&tx).unwrap();
    println!("{}", json);
    Ok(())
}
